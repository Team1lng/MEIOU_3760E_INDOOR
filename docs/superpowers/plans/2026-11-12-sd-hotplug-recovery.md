# SD Hotplug Recovery Implementation Plan

> For agentic workers: execute the tasks in order in the current authorized workspace; use independent code review at the completed-change checkpoint. Do not commit, package, deploy, or proceed beyond the user's approval checkpoint.

**Goal:** Recover SD access after device re-enumeration without retaining the removed device's mount or writing a pending request to a replacement card.

**Architecture:** Use kernel mount metadata and device identity in the SD worker. Hold the unavailable state until UI cleanup is acknowledged, retry normal unmount on busy resources, and gate asynchronous persistence with a media generation and an I/O usage count.

**Tech Stack:** C11, pthreads, Linux 4.4 mount/stat APIs, LVGL event queue, native GCC regression harness, ARM uClibc GCC 4.9.4.

## Tasks

- [x] Add `tests/sd_hotplug/test_command_execution.c` and native runner. Run `python3 tests/sd_hotplug/run_tests.py command`; exit codes 1 and 32 must fail the old helper's success contract.
- [x] Fix `common/Amy_sdk.c` to combine callback results with `pclose()` status, preserving successful read-filter and stdin-writer behavior. Re-run the command tests.
- [x] Add hotplug regression fixtures for old `179:1` mount versus current `179:9`, busy unmount, delayed UI acknowledgement, missing mounts, and failed mount. Run the tests against the original state machine and retain the failing results.
- [x] Replace shell mount/df probing in `common/Amy_tfcard.c` with mountinfo parsing, validated block nodes, normal mount/umount and statvfs. Add synchronized status, event sequence/acknowledgement, retry and persistence-generation APIs in `common/Amy_tfcard.h`.
- [x] Integrate sequence payloads and a UI cleanup hook in `common/Amy_lvgl.c/.h`; report queue insertion failures. Keep existing event APIs compatible. Cover dropped events and acknowledgement ordering in the native tests.
- [x] Register SD cleanup in `project/page/page_power.c` and `page_common.c/.h`. Stop SD-backed recording, music and photo-frame work on the UI thread; reuse existing playback/list page cleanup. Add standby-state bookkeeping without closing the live RTSP stream.
- [x] Protect `project/monitor.c` snapshot persistence with SD generation and I/O ownership; clear the running flag on every completion path. Check record-stop ownership and fix only cleanup defects required by this path.
- [x] Run `python3 tests/sd_hotplug/run_tests.py`, cross-compile changed modules and relevant project objects, then perform independent code review. Fix scope-related defects and repeat affected checks.
- [x] Save the complete task diff, baseline manifest, verification output and calculated SHA-256; provide the six-part approval report and keep the candidate stable while awaiting approval.

## Delivery checkpoint

- Candidate code and review artifacts are prepared in the authorized workspace.
- Native regression: 100 checks pass; ARM common/avi/live and changed project objects compile with the project warning policy.
- Hardware acceptance and firmware packaging/installation have not been performed.
- [ ] User approval of the content-bound candidate; no commit, merge or deployment is implied.
