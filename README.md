# Yamt
Yet Another Macro Tool


Quick Wins

Repeat Count: Add a numeric input to repeat each injection N times; loop SendParsedInput.
Start/Stop Hotkeys: Add configurable global hotkeys (use RegisterHotKey) and UI to change them.
Save/Load Presets: Save text + sliders to a simple JSON/INI file for quick reuse.
Clipboard Send: Add a “Send Clipboard” button that injects current clipboard text.
Medium Effort

Target Window Selection: Let user pick a target window (by title/handle) and use AttachThreadInput/SetForegroundWindow before sending.
Sequence/Macro Editor: Allow a list of steps (text, delay, repeat, conditional) and run them in order.
Per-step Timing & Jitter: Allow custom base/jitter per sequence step instead of global sliders.
Profile Persistence: Store UI state (window size, last preset, theme) in %APPDATA%.
Advanced Ideas

Scripting Support: Embed Lua or JS so users can script conditional flows, loops, and window logic.
Conditional Triggers: Start macros on events (window appears, pixel color, clipboard change).
Plugin API: Design a simple plugin interface (DLL or script) so third-party extensions can add token handlers.
Cross-platform CLI Mode: Add a command-line mode to run scripts/presets without UI (useful for automation).
Safety & UX

Confirmation & Emergency Stop: Add an abort hotkey and an optional confirmation delay before starting.
Activity Logging: Log injections with timestamps for debugging and audits.
Accessibility Options: Larger fonts, readouts, and keyboard navigation for the UI.
Next step: pick 1–2 items to implement first and I’ll prepare a focused plan and patches (e.g., add presets + repeat count). Which do you want to start with?