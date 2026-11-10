#!/usr/bin/env python3
"""
Phase Gate Verification Script for OpenVINO Local LLM Workbench
Enforces strict phase gating according to the Implementation Blueprint.
"""

import sys
import os
import re
import argparse
import subprocess
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
PHASE_STATUS_FILE = REPO_ROOT / "docs" / "PHASE_STATUS.md"
PHASE_REPORTS_DIR = REPO_ROOT / "docs" / "phase-reports"

PHASE_NAMES = [
    "00", "01", "02", "03", "04", "05", "06", "07", "08", "09"
]

def parse_phase_status(file_path):
    if not file_path.exists():
        return {}
    content = file_path.read_text(encoding="utf-8")
    statuses = {}
    # Parse markdown table rows: | Phase | Status | Local tests | Commit | Remote verified | User approval |
    for line in content.splitlines():
        line = line.strip()
        if not line.startswith("|"):
            continue
        parts = [p.strip() for p in line.split("|")[1:-1]]
        if len(parts) >= 6:
            phase_raw = parts[0].replace("*", "").strip()
            # Match phase number e.g. "00 --- Workspace audit" or "00"
            m = re.match(r"^(\d{2})", phase_raw)
            if m:
                phase_id = m.group(1)
                statuses[phase_id] = {
                    "status": parts[1],
                    "local_tests": parts[2],
                    "commit": parts[3],
                    "remote_verified": parts[4],
                    "user_approval": parts[5],
                }
    return statuses

def check_git_remote_sync(expected_branch="main"):
    try:
        head_commit = subprocess.check_output(
            ["git", "rev-parse", "HEAD"], cwd=REPO_ROOT, text=True
        ).strip()
    except subprocess.CalledProcessError:
        return False, "Git repository has no commits yet"

    try:
        remote_output = subprocess.check_output(
            ["git", "ls-remote", "origin", f"refs/heads/{expected_branch}"],
            cwd=REPO_ROOT, text=True
        ).strip()
    except subprocess.CalledProcessError as e:
        return False, f"git ls-remote failed: {e}"

    if not remote_output:
        return False, f"No remote branch 'refs/heads/{expected_branch}' found on origin"

    remote_commit = remote_output.split()[0]
    if head_commit != remote_commit:
        return False, f"Local HEAD ({head_commit}) does not match remote ({remote_commit})"

    return True, head_commit

def verify_phase(target_phase_str, require_complete=False):
    target_idx = int(target_phase_str)
    statuses = parse_phase_status(PHASE_STATUS_FILE)

    if not statuses:
        print(f"[ERROR] Cannot parse phase statuses from {PHASE_STATUS_FILE}")
        return False

    # Check that all prior phases are COMPLETE
    for i in range(target_idx):
        prev_phase_id = f"{i:02d}"
        prev_data = statuses.get(prev_phase_id)
        if not prev_data:
            print(f"[FAIL] Missing status entry for prerequisite Phase {prev_phase_id}")
            return False
        if prev_data["status"] != "COMPLETE":
            print(f"[FAIL] Prerequisite Phase {prev_phase_id} is not COMPLETE (Status: {prev_data['status']})")
            return False

    target_id = f"{target_idx:02d}"
    target_data = statuses.get(target_id)
    if not target_data:
        print(f"[FAIL] Missing status entry for target Phase {target_id}")
        return False

    # Check phase report existence
    report_file = PHASE_REPORTS_DIR / f"PHASE_{target_id}_REPORT.md"
    if not report_file.exists():
        print(f"[FAIL] Missing required report file: {report_file}")
        return False

    report_content = report_file.read_text(encoding="utf-8")
    report_content_lower = report_content.lower()
    for req_field in ["phase:", "status:", "environment:", "test results", "acceptance criteria checklist"]:
        if req_field not in report_content_lower:
            print(f"[FAIL] Phase report {report_file.name} is missing required section: '{req_field}'")
            return False

    if require_complete:
        if target_data["status"] != "COMPLETE":
            print(f"[FAIL] Phase {target_id} is marked '{target_data['status']}', expected 'COMPLETE'")
            return False
        if target_data["commit"] in ["---", "PENDING", ""]:
            print(f"[FAIL] Phase {target_id} has no valid commit recorded in {PHASE_STATUS_FILE}")
            return False
        if target_data["user_approval"] != "YES":
            print(f"[FAIL] Phase {target_id} lacks explicit user approval in {PHASE_STATUS_FILE}")
            return False

        # Remote check
        is_synced, msg = check_git_remote_sync()
        if not is_synced:
            print(f"[FAIL] Remote synchronization check failed: {msg}")
            return False

    print(f"[OK] Phase {target_id} gate validation passed.")
    print(f"     Status: {target_data['status']}")
    print(f"     Local tests: {target_data['local_tests']}")
    print(f"     Commit: {target_data['commit']}")
    print(f"     Remote verified: {target_data['remote_verified']}")
    print(f"     User approval: {target_data['user_approval']}")
    return True

def main():
    parser = argparse.ArgumentParser(description="Verify phase gates.")
    parser.add_argument("--phase", required=True, help="Phase number to verify (e.g. 0, 00, 1, 01)")
    parser.add_argument("--require-complete", action="store_true", help="Require target phase to be fully COMPLETE")
    args = parser.parse_args()

    phase_num = int(args.phase)
    success = verify_phase(f"{phase_num:02d}", require_complete=args.require_complete)
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
