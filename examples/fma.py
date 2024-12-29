from cycler import cycler
from datetime import datetime, timezone
from fire import Fire
from matplotlib import pyplot as plt
from matplotlib.lines import Line2D
from statistics import mean, median
from subprocess import check_output
from tqdm import tqdm
from random import shuffle
import itertools
import json
import os

def parse_option(option):
    try:
        parts = option.split(",")
    except AttributeError:
        try:
            _ = iter(option)
            parts = option
        except:
            parts = [option]
    values = []
    for part in parts:
        try:
            start, end = part.split("-")
            if ":" in end:
                end, step = end.split(":")
            else:
                step = 1
            start, end, step = map(int, (start, end, step))
            values += list(range(start, end+1, step))
        except:
            values.append(int(part))
    return values

def run(binary, count, processor):
    if processor > 1:
        output = check_output(["taskset", "-c", f"0-{processor-1}", binary, str(count), str(processor)])
    elif processor == 1:
        output = check_output(["taskset", "1", binary, str(count), str(processor)])
    else:
        output = check_output([binary, str(count), str(processor)])
    _, t = output.split()
    return float(t)

def load_experiments(load):
    timings = {}
    with open(load, "r") as f:
        json_data = json.load(f)
    for binary, data in json_data.items():
        for processor, data in data.items():
            for count, runs in data.items():
                for repeat, run in enumerate(runs):
                    timings[(binary, int(count), int(processor), repeat)] = run

    return timings

def save_experiments(data, timings, binaries, counts, processors, repeats):
    json_data = {}
    for binary in binaries:
        json_data[binary] = {}
        for processor in processors:
            json_data[binary][processor] = {}
            for count in counts:
                json_data[binary][processor][count] = []
                for repeat in range(repeats):
                    json_data[binary][processor][count].append(timings[(binary, count, processor, repeat)])
    with open(data, "w") as f:
        json.dump(json_data, f)

def run_experiments(progress, binaries, counts, processors, repeats):
    timings = {}
    run_args = list(itertools.product(binaries, counts, processors, range(repeats)))
    shuffle(run_args)
    for binary, count, processor, repeat in progress(run_args):
        timings[(binary, count, processor, repeat)] = run(binary, count, processor)

    return timings

now = f"{datetime.now(timezone.utc).astimezone():%Y-%m-%d-%H%M%S}"

def main(counts, processors, *binaries, repeats=1, time_per_item=False, metric="median", logx=False, logy=False, log_log=False, grid=True, legend=False, prefix=f"{now}-", plot="comparison.pdf", data="comparison.json", load=False, load_gpu=False, quiet=False):
    if quiet:
        progress = lambda x: x
    else:
        progress = tqdm
    metric = metric.upper()
    if metric == "MEAN":
        metric = mean
    elif metric == "MEDIAN":
        metric = median
    else:
        raise ValueError(f"Unknown metric: {metric}")

    counts = parse_option(counts)
    processors = parse_option(processors)

    if isinstance(load, str):
        data = False
        timings = load_experiments(load)
        if load_gpu:
            gpu_timings = load_experiments(load_gpu)
            gpu_binaries = set()
            for (binary, count, processor, repeat), run in gpu_timings.items():
                binary = f"{binary}-gpu"
                gpu_binaries.add(binary)
                assert processor == -1
                assert 0 in processors
                for other_processor in processors:
                    timings[(binary, count, other_processor, repeat)] = run if other_processor == 0 else 0
            binaries = list(binaries) + list(gpu_binaries)
    else:
        timings = run_experiments(progress, binaries, counts, processors, repeats)

    def compute_metric(time, count):
        if time_per_item:
            return metric(time) / count
        else:
            return metric(time)

    average_timings = { (binary, count, processor) : compute_metric(list(timings[(binary, count, processor, i)] for i in range(repeats)), count) for binary in binaries for count in counts for processor in processors}

    if plot:
        if log_log:
            if not logx:
                logx = log_log
            if not logy:
                logy = log_log
        del log_log
        if plot:
            plot = prefix + plot
        if data:
            data = prefix + data
        del prefix
        if legend:
            if isinstance(legend, (tuple, list)):
                assert len(legend) == len(binaries)
            else:
                assert isinstance(legend, bool)
                legend = []
                for binary in binaries:
                    legend.append(os.path.splitext(os.path.basename(binary))[0])

        def make_linestyle(n):
            if n > 0:
                pattern = tuple()
                for _ in range(n):
                    pattern += (1, 3)
                pattern += (10, 3)
                return (0, pattern)
            else:
                return "solid"

        color_cycler = cycler(color=plt.cm.tab10.colors)
        line_cycler = cycler(linestyle=[make_linestyle(processor) for processor in processors])

        fig, ax = plt.subplots(1,1)
        ax.set_prop_cycle(color_cycler * line_cycler)

        for binary in binaries:
            for processor in processors:
                x = counts
                y = [average_timings[(binary, count, processor)] for count in counts]
                if 0 not in y:
                    ax.plot(x, y)

        if logx:
            if isinstance(logx, int):
                ax.set_xscale("log", base=logx)
            else:
                ax.set_xscale("log")
        if logy:
            if isinstance(logy, int):
                ax.set_yscale("log", base=logy)
            else:
                ax.set_yscale("log")

        if grid:
            ax.grid()

        if legend:
            lines = []
            labels = []
            for label, binary, color in zip(legend, binaries, color_cycler):
                line = Line2D([0], [0], **color)
                lines.append(line)
                labels.append(label)
            for processor, linestyle in zip(processors, line_cycler):
                line = Line2D([0], [0], color="k", **linestyle)
                if processor > 1:
                    label = f"{processor} processors"
                elif processor == 1:
                    label = "1 processor"
                else:
                    label = "all processors"
                lines.append(line)
                labels.append(label)
            ax.legend(lines, labels)

        fig.tight_layout()
        fig.savefig(plot, format="pdf", bbox_inches="tight")

    if data:
        save_experiments(data, timings, binaries, counts, processors, repeats)
        if not quiet:
            print(os.path.abspath(data))

if __name__ == "__main__":
    Fire(main)
