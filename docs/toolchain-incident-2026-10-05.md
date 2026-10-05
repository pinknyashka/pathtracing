# Toolchain incident: gcc 15 frontends crash with `0xC00000FD` (2026-10-05)

Status: **RESOLVED** with an offline fix (no msys2 package changes). Prevention runbook in section 6.

## 1. Symptoms

- `cc1plus.exe` / `cc1.exe` (`C:\msys64\ucrt64\lib\gcc\x86_64-w64-mingw32\15.2.0\`) died at startup with exit `0xC00000FD` (`STATUS_STACK_BUFFER_OVERRUN`, i.e. `__fastfail`), no diagnostics — even `cc1plus --version` (which prints nothing but exits 0 — a cosmetic quirk, not the bug).
- `gcc`/`g++` drivers, `as`, and `ld` all ran fine → looked like "only the C/C++ frontends are broken".
- `cmake --build build` (Ninja) reported FAILED with **no compiler output**; pre-existing binaries in `build/` looked fine but were stale (built before the breakage).
- Worked at 13:55 on 2026-10-05; broken by end of session.

## 2. First (wrong) hypothesis — a dead end

- Date heuristic: `libwinpthread-1.dll` (21.08.2025) was newer than the gcc 15.2.0 Rev8 package files (09.08.2025) → suspected a bad mingw-w64-crt/gcc package update; plan was to reinstall both packages via `pacman -Sy`.
- Dead end: no VPN → all msys2 mirrors timed out; reinstallation was impossible. The fix had to be **offline**.

## 3. Root cause

- The machine PATH contains `D:\Projects\__tools\mingw_7_2_0\mingw64\bin` — a 2017-era standalone MinGW kept for other tooling — which ships old `libgmp-10.dll`, `libmpfr-6.dll`, `libmpc-3.dll`, `libwinpthread-1.dll` (2017-11-10, 51,712 bytes) plus old binutils.
- Before the fix, `C:\msys64\ucrt64\bin` was **not on PATH at all** (g++ was invoked by absolute path). cc1/cc1plus resolve their runtime DLLs via the standard Windows search order (system dirs, then PATH) → they loaded the 2017 GMP/MPFR/MPC pair from `__tools\mingw_7_2_0`, ABI-incompatible with the GCC 15 frontend → stack-canary fastfail at startup.
- `as`/`ld` "worked" because the driver's canonical path (`C:\msys64\x86_64-w64-mingw32\bin\as.exe`) does not exist under ucrt64, so it silently fell back to PATH and picked up the 2017 binutils — which happened to be link-compatible.
- The msys2 packages were fine the whole time; the date skew in section 2 was a red herring (the newer libwinpthread is a normal msys2 package, not a bad update).

## 4. Fix applied (offline, 2026-10-05)

1. **User PATH**: prepended `C:\msys64\ucrt64\bin` (persisted, user scope). New shells find the right `as`/`ld` first; anything invoking g++ by absolute path now also resolves DLLs correctly.
2. **DLL staging** (backstop for sessions whose PATH is still stale — the exe directory wins Windows DLL search): copied from `C:\msys64\ucrt64\bin` into `C:\msys64\ucrt64\lib\gcc\x86_64-w64-mingw32\15.2.0\`:
   `libwinpthread-1.dll`, `libgcc_s_seh-1.dll`, `libgmp-10.dll`, `libmpfr-6.dll`, `libmpc-3.dll`, `libisl-23.dll`, `zlib1.dll`, `libzstd.dll`.
3. Restored `C:\msys64\etc\pacman.d\mirrorlist.ucrt64` and `mirrorlist.msys` from their `.bak` files (they had been narrowed to two mirrors while probing; note the msys repo lives at `/msys/x86_64/`, not `/mingw/ucrt64/`).

## 5. Verification

- Fresh shell (user PATH + machine PATH): `g++ --version` → `g++.exe (Rev8, Built by MSYS2 project) 15.2.0`; C++20 smoke compile + link + run → exit 0, correct output.
- `build/` wiped and reconfigured (`D:\Projects\__tools\cmake` + Ninja + `-DCMAKE_CXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe`); full build of all targets; ctest: 4/5 pass (the one failure is the app-level `test_tracer` threshold, see AGENTS.md — unrelated to the toolchain).

## 6. Prevention runbook

1. **Triage order for frontend crashes**: check PATH order and DLL shadowing *before* touching msys2 packages. Fast check: is `C:\msys64\ucrt64\bin` ahead of every other `*\mingw64\bin` on `$env:PATH`? If not, prepend it and retry — that alone fixed this incident.
2. **One-line health check** (run after any msys2 update, PATH change, or mysterious frontend crash):

   ```powershell
   $u=[Environment]::GetEnvironmentVariable("Path","User"); $m=[Environment]::GetEnvironmentVariable("Path","Machine"); $env:PATH="$u;$m"
   Set-Content $env:TEMP\smoke.cpp @('#include <iostream>','int main(){std::cout<<"ok";return 0;}')
   g++ -std=c++20 $env:TEMP\smoke.cpp -o $env:TEMP\smoke.exe; "compile=$LASTEXITCODE"; & "$env:TEMP\smoke.exe"
   ```

   Expect `compile=0` + `ok`. Anything else means a DLL/PATH problem, not a compiler or code problem.
3. **PATH hygiene**: keep `C:\msys64\ucrt64\bin` first in the *User* PATH permanently. Do not add other mingw/binutils trees to the *Machine* PATH; if one must stay (`__tools\mingw_7_2_0`), user-PATH ordering is the only guard — re-verify it after anything that edits PATH.
4. **After any `pacman -Syu` / gcc reinstall**: rerun the health check; if the frontend version directory changes (e.g. `15.2.0` → `16.x`), re-stage the 8 DLLs from `C:\msys64\ucrt64\bin` into the new `lib\gcc\x86_64-w64-mingw32\<ver>\` directory, update `-DCMAKE_CXX_COMPILER` if the path moved, and wipe `build/` before trusting any results.
5. **Stale-session gotcha**: long-running processes (including agent CLIs) keep the PATH from when they started. In every shell command, re-derive it with the two lines from step 2 before running anything toolchain-related.
6. **Offline rule**: no VPN → msys2 mirrors and GitHub are unreachable. Never plan a fix that needs `pacman` or a push; prefer PATH/DLL-level fixes that work offline.
7. **Never trust `build/` across a toolchain change**: wipe + reconfigure. Ninja "FAILED with no compiler output" is the signature of a frontend crash, not a code error — old binaries will still run and mislead you.

## 7. Evidence

- Shadowing copy: `D:\Projects\__tools\mingw_7_2_0\mingw64\bin\libwinpthread-1.dll` — 51,712 bytes, 2017-11-10.
- Healthy copy: `C:\msys64\ucrt64\bin\libwinpthread-1.dll` — 63,675 bytes, 2025-08-21 (staged next to the frontends; see AGENTS.md "Toolchain notes").
- `0xC00000FD` = `__fastfail(FAIL_FAST_STACK_COOKIE)` — the classic canary trip from an ABI mismatch between a binary and its math libraries.
- Known-good combination: gcc 15.2.0 Rev8 frontends + ucrt64 runtime DLLs (winpthreads r124, gmp/mpfr/mpc as packaged).
