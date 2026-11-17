.pragma library

function action(ids, state) {
    const keys = (ids || []).map(id => String(id).replace(/[{}]/g, ""));
    const windows = (state.windows || []).filter(window => keys.indexOf(window.id) >= 0);
    if (!state.enabled || !windows.length) return null;
    if (windows.length > 1) return { method: "ToggleOverview", args: [] };
    const window = windows[0];
    return { method: state.activeId === window.id && !window.minimized ? "Minimize" : "Activate", args: [window.id] };
}
