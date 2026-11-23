# Performance

MNE-CPP is built for real-time functional brain imaging. Data arrive from the amplifier, pass through
filters, projections and source estimates, and reach the screen while the subject is still in the scanner.
This page shows how fast that path is, with numbers you can reproduce.

**How the numbers are made**

- Every number comes from a script in the repository, `scripts/bench/run_benchmarks.py`. Every chart names the
  machine, operating system, compiler and commit it was measured on, and the command that measures it again.
- Comparisons with MNE-Python use the same files and the same parameters: the MNE sample data set, the same
  source space, BEM and covariance. Where MNE-Python can run in parallel (`n_jobs`), the chart says how it ran.
- Times are medians of several runs. A timed span starts before the inputs are read and ends after the result
  is written, for both programs. Start-up times are measured separately: Python's interpreter and imports are
  part of what a user waits for, but they are not the computation.
- Acquisition has no MNE-Python counterpart, because MNE-Python does not acquire data. Those charts show
  absolute rates and latencies, measured by `mne_acq_bench`, which checks every sample it moves.

**Limits of these numbers**

- They come from one machine. CI measures again on every push to staging and publishes the results as an
  artifact, but shared CI machines vary by tens of percent from run to run.
- Wake-up latencies depend on the operating system's scheduler. The p99.9 values show what the scheduler does
  under load as much as what MNE-CPP does.
- Still to come: end-to-end latency from device to display, real-time filtering and inverse solutions per
  sample block, and 3D rendering rates over source-space size.

To refresh this page, run `scripts/bench/refresh.sh`. It measures, writes
`doc/website/static/bench/results.json`, renders the charts and regenerates this page.
