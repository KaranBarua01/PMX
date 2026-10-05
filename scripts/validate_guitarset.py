#!/usr/bin/env python3
import argparse
import json
import math
import pathlib
import re
import subprocess
import sys


def note_events(jams_path):
    doc=json.loads(pathlib.Path(jams_path).read_text(encoding="utf-8"))
    events=[]
    for ann in doc.get("annotations",[]):
        namespace=ann.get("namespace","")
        if namespace!="note_midi":
            continue
        for obs in ann.get("data",[]):
            value=obs.get("value")
            if isinstance(value,dict):
                value=value.get("midi",value.get("value"))
            try:
                midi=int(round(float(value)))
                start=float(obs.get("time",0.0))
                duration=float(obs.get("duration",0.0))
            except (TypeError,ValueError):
                continue
            if 0<=midi<=127 and duration>0.0:
                events.append((start,start+duration,midi))
    return events


def boundary_near(events,t,tolerance=0.055):
    for start,end,_ in events:
        if abs(t-start)<=tolerance or abs(t-end)<=tolerance:
            return True
    return False


def active_notes(events,t):
    return {midi for start,end,midi in events if start<=t<end}


def parse_probe_line(line):
    fields=line.strip().split(",")
    if len(fields)<2:
        return None
    t=float(fields[0])
    count=int(fields[1])
    notes=set()
    for item in fields[2:2+count]:
        if not item:
            continue
        midi=item.split(":",1)[0]
        notes.add(int(midi))
    return t,notes


def run_probe(probe,wav_path):
    proc=subprocess.run([str(probe),str(wav_path)],text=True,capture_output=True)
    if proc.returncode!=0:
        raise RuntimeError(f"probe failed for {wav_path.name}: {proc.stderr.strip()}")
    frames=[]
    for line in proc.stdout.splitlines():
        parsed=parse_probe_line(line)
        if parsed is not None:
            frames.append(parsed)
    return frames


def track_style(stem):
    parts=stem.split("_")
    if len(parts)<2:
        return "unknown"
    token=parts[1].split("-",1)[0]
    match=re.match(r"([A-Za-z]+)",token)
    return match.group(1) if match else token


def choose_tracks(wavs):
    by_player={}
    for wav in wavs:
        stem=wav.stem
        player=stem.split("_",1)[0]
        kind="comp" if stem.endswith("_comp") else ("solo" if stem.endswith("_solo") else None)
        if kind is None:
            continue
        by_player.setdefault(player,{"comp":[],"solo":[]})[kind].append(wav)

    selected=[]
    for player in sorted(by_player):
        groups=by_player[player]
        comps=sorted(groups["comp"],key=lambda p:(track_style(p.stem),p.stem))
        solos=sorted(groups["solo"],key=lambda p:(track_style(p.stem),p.stem))

        # Two comp excerpts with different styles when possible.
        comp_choices=[]
        seen=set()
        for wav in comps:
            style=track_style(wav.stem)
            if style in seen:
                continue
            comp_choices.append(wav)
            seen.add(style)
            if len(comp_choices)==2:
                break
        if len(comp_choices)<2:
            for wav in comps:
                if wav not in comp_choices:
                    comp_choices.append(wav)
                if len(comp_choices)==2:
                    break

        # One solo excerpt per player.
        solo_choice=solos[len(solos)//2] if solos else None
        selected.extend(comp_choices)
        if solo_choice is not None:
            selected.append(solo_choice)
    return selected


def score_track(wav,jams,probe):
    events=note_events(jams)
    if not events:
        raise RuntimeError(f"no note_midi annotations in {jams.name}")

    frames=run_probe(probe,wav)
    tp=fp=fn=exact=used=0
    single_frames=false_poly=0
    predicted_total=0

    for t,predicted in frames:
        if boundary_near(events,t):
            continue
        truth=active_notes(events,t)
        if not truth and not predicted:
            continue

        used+=1
        predicted_total+=len(predicted)
        tp+=len(truth & predicted)
        fp+=len(predicted-truth)
        fn+=len(truth-predicted)
        if truth==predicted:
            exact+=1
        if len(truth)<=1:
            single_frames+=1
            if len(predicted)>1:
                false_poly+=1

    precision=tp/(tp+fp) if tp+fp else 0.0
    recall=tp/(tp+fn) if tp+fn else 0.0
    f1=2*precision*recall/(precision+recall) if precision+recall else 0.0

    return {
        "file":wav.name,
        "kind":"comp" if wav.stem.endswith("_comp") else "solo",
        "style":track_style(wav.stem),
        "frames":used,
        "tp":tp,"fp":fp,"fn":fn,
        "precision":precision,
        "recall":recall,
        "f1":f1,
        "exact_frame_rate":exact/used if used else 0.0,
        "false_polyphonic_rate_when_gt_le_1":false_poly/single_frames if single_frames else 0.0,
        "mean_predicted_notes":predicted_total/used if used else 0.0,
    }


def aggregate(rows):
    tp=sum(r["tp"] for r in rows)
    fp=sum(r["fp"] for r in rows)
    fn=sum(r["fn"] for r in rows)
    frames=sum(r["frames"] for r in rows)
    precision=tp/(tp+fp) if tp+fp else 0.0
    recall=tp/(tp+fn) if tp+fn else 0.0
    f1=2*precision*recall/(precision+recall) if precision+recall else 0.0
    exact=sum(r["exact_frame_rate"]*r["frames"] for r in rows)
    return {
        "tracks":len(rows),
        "frames":frames,
        "precision":precision,
        "recall":recall,
        "f1":f1,
        "weighted_exact_frame_rate":exact/frames if frames else 0.0,
    }


def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--audio-dir",required=True,type=pathlib.Path)
    ap.add_argument("--annotation-dir",required=True,type=pathlib.Path)
    ap.add_argument("--probe",required=True,type=pathlib.Path)
    ap.add_argument("--report",required=True,type=pathlib.Path)
    args=ap.parse_args()

    wavs=sorted(args.audio_dir.rglob("*.wav"))
    jams_by_stem={p.stem:p for p in args.annotation_dir.rglob("*.jams")}
    selected=choose_tracks(wavs)
    if len(selected)<12:
        raise RuntimeError(f"expected at least 12 representative tracks, got {len(selected)}")

    rows=[]
    for wav in selected:
        jams=jams_by_stem.get(wav.stem)
        if jams is None:
            raise RuntimeError(f"annotation missing for {wav.name}")
        row=score_track(wav,jams,args.probe)
        rows.append(row)
        print(f"{row['kind']:4} {row['style']:5} {wav.name:32} "
              f"P={row['precision']:.3f} R={row['recall']:.3f} F1={row['f1']:.3f} "
              f"exact={row['exact_frame_rate']:.3f}")

    comp=[r for r in rows if r["kind"]=="comp"]
    solo=[r for r in rows if r["kind"]=="solo"]
    report={
        "selection_policy":"2 comp styles + 1 solo per player",
        "overall":aggregate(rows),
        "comp":aggregate(comp),
        "solo":aggregate(solo),
        "tracks":rows,
    }
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(report,indent=2),encoding="utf-8")

    print("\nOVERALL",json.dumps(report["overall"],sort_keys=True))
    print("COMP   ",json.dumps(report["comp"],sort_keys=True))
    print("SOLO   ",json.dumps(report["solo"],sort_keys=True))
    return 0


if __name__=="__main__":
    sys.exit(main())
