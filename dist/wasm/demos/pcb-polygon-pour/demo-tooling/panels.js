const VALID_STATES = new Set(["hidden", "collapsed", "open"]);
const VALID_SIDES = new Set(["left", "right", "bottom"]);
function finiteOption(value, fallback) {
    return Number.isFinite(value) ? Number(value) : fallback;
}
function customEvent(type, detail) {
    return new CustomEvent(type, { detail });
}
export class PanelManager {
    root;
    entries = new Map();
    dom = new Map();
    cleanupCallbacks = [];
    sizes;
    options;
    destroyed = false;
    constructor(root, options = {}) {
        if (!(root instanceof HTMLElement))
            throw new TypeError("PanelManager root must be an HTMLElement.");
        this.root = root;
        this.options = {
            leftSize: finiteOption(options.leftSize, 300),
            rightSize: finiteOption(options.rightSize, 340),
            bottomSize: finiteOption(options.bottomSize, 240),
            railSize: finiteOption(options.railSize, 28),
            minimumSideSize: finiteOption(options.minimumSideSize, 200),
            maximumSideSize: finiteOption(options.maximumSideSize, 640),
            minimumBottomSize: finiteOption(options.minimumBottomSize, 140),
            maximumBottomSize: finiteOption(options.maximumBottomSize, 600),
        };
        this.sizes = {
            left: this.options.leftSize,
            right: this.options.rightSize,
            bottom: this.options.bottomSize,
        };
        this.root.classList.add("gdm-panel-root");
        this.buildSide("left");
        this.buildSide("right");
        this.buildSide("bottom");
        this.updateLayout();
    }
    register(panel, initialState = "hidden") {
        this.assertActive();
        const id = String(panel.id || "").trim();
        const title = String(panel.title || "").trim();
        if (!id || !title)
            throw new TypeError("Panels require non-empty id and title values.");
        if (this.entries.has(id))
            throw new Error(`Panel ${id} is already registered.`);
        const side = VALID_SIDES.has(panel.side ?? "right") ? (panel.side ?? "right") : "right";
        const sideDom = this.requireSide(side);
        sideDom.rail.hidden = false;
        const tab = document.createElement("button");
        tab.type = "button";
        tab.className = "gdm-panel-tab";
        tab.dataset.panelId = id;
        tab.textContent = title;
        tab.title = title;
        tab.setAttribute("aria-expanded", "false");
        sideDom.rail.append(tab);
        const wrapper = document.createElement("section");
        wrapper.className = "gdm-panel";
        wrapper.dataset.panelId = id;
        wrapper.setAttribute("aria-label", title);
        const header = document.createElement("header");
        header.className = "gdm-panel-header";
        const heading = document.createElement("span");
        heading.className = "gdm-panel-title";
        heading.textContent = title;
        header.append(heading);
        const actions = document.createElement("span");
        actions.className = "gdm-panel-actions";
        const collapseButton = document.createElement("button");
        collapseButton.type = "button";
        collapseButton.className = "gdm-panel-button";
        collapseButton.textContent = "−";
        collapseButton.title = "Collapse";
        collapseButton.setAttribute("aria-label", `Collapse ${title}`);
        const closeButton = document.createElement("button");
        closeButton.type = "button";
        closeButton.className = "gdm-panel-button";
        closeButton.textContent = "×";
        closeButton.title = "Hide";
        closeButton.setAttribute("aria-label", `Hide ${title}`);
        actions.append(collapseButton, closeButton);
        header.append(actions);
        const body = document.createElement("div");
        body.className = "gdm-panel-body";
        wrapper.append(header, body);
        panel.mount(body);
        const entry = {
            panel,
            side,
            tab,
            wrapper,
            body,
            collapseButton,
            state: "hidden",
        };
        this.entries.set(id, entry);
        tab.addEventListener("click", () => {
            if (entry.state === "open")
                this.focus(id);
            else
                this.setPanelState(id, "open");
        });
        collapseButton.addEventListener("click", () => {
            this.setPanelState(id, entry.state === "collapsed" ? "open" : "collapsed", {
                focus: false,
            });
        });
        closeButton.addEventListener("click", () => this.setPanelState(id, "hidden"));
        this.setPanelState(id, VALID_STATES.has(initialState) ? initialState : "hidden");
    }
    unregister(panelId) {
        this.assertActive();
        const entry = this.entries.get(panelId);
        if (!entry)
            return false;
        entry.tab.remove();
        entry.wrapper.remove();
        entry.panel.destroy?.();
        this.entries.delete(panelId);
        if (![...this.entries.values()].some((candidate) => candidate.side === entry.side)) {
            this.requireSide(entry.side).rail.hidden = true;
        }
        this.updateSide(entry.side);
        return true;
    }
    setPanelState(panelId, requestedState, options = {}) {
        this.assertActive();
        const entry = this.entries.get(panelId);
        if (!entry)
            throw new Error(`Unknown panel ${panelId}.`);
        const state = VALID_STATES.has(requestedState) ? requestedState : "hidden";
        const shouldFocus = options.focus !== false;
        const dock = this.requireSide(entry.side).dock;
        if (entry.state === state) {
            if (shouldFocus && state !== "hidden")
                this.focus(panelId);
            return;
        }
        entry.state = state;
        entry.tab.classList.toggle("active", state !== "hidden");
        entry.tab.setAttribute("aria-expanded", String(state === "open"));
        entry.wrapper.classList.toggle("collapsed", state === "collapsed");
        entry.wrapper.hidden = state === "hidden";
        if (state === "hidden") {
            entry.wrapper.remove();
        }
        else if (entry.wrapper.parentElement !== dock) {
            dock.append(entry.wrapper);
        }
        this.syncCollapseButton(entry);
        if (shouldFocus && state !== "hidden")
            this.focus(panelId);
        entry.panel.onStateChange?.(state);
        this.updateSide(entry.side);
    }
    getPanelState(panelId) {
        return this.entries.get(panelId)?.state ?? null;
    }
    getPanelStates() {
        return Object.freeze(Object.fromEntries([...this.entries].map(([id, entry]) => [id, entry.state])));
    }
    applyPanelStates(states) {
        this.assertActive();
        for (const id of this.entries.keys())
            this.setPanelState(id, states[id] ?? "hidden", { focus: false });
    }
    focus(panelId) {
        const entry = this.entries.get(panelId);
        if (!entry || entry.state === "hidden")
            return;
        const dock = this.requireSide(entry.side).dock;
        const firstPanel = dock.querySelector(":scope > .gdm-panel");
        if (firstPanel !== entry.wrapper)
            dock.insertBefore(entry.wrapper, firstPanel);
    }
    refresh(panelIds) {
        this.assertActive();
        const requested = panelIds ? new Set(panelIds) : null;
        for (const [id, entry] of this.entries) {
            if (entry.state === "hidden" || (requested && !requested.has(id)))
                continue;
            entry.body.replaceChildren();
            entry.panel.mount(entry.body);
        }
    }
    destroy() {
        if (this.destroyed)
            return;
        for (const callback of this.cleanupCallbacks.splice(0))
            callback();
        for (const entry of this.entries.values())
            entry.panel.destroy?.();
        this.entries.clear();
        for (const side of this.dom.values()) {
            side.rail.remove();
            side.dock.remove();
        }
        this.dom.clear();
        this.root.classList.remove("gdm-panel-root");
        for (const name of ["--gdm-content-left", "--gdm-content-right", "--gdm-content-bottom"])
            this.root.style.removeProperty(name);
        this.destroyed = true;
    }
    buildSide(side) {
        const rail = document.createElement("nav");
        rail.className = `gdm-panel-rail gdm-panel-rail--${side}`;
        rail.setAttribute("aria-label", `${side} demo panels`);
        rail.hidden = true;
        const dock = document.createElement("aside");
        dock.className = `gdm-panel-dock gdm-panel-dock--${side}`;
        dock.hidden = true;
        const resize = document.createElement("div");
        resize.className = "gdm-panel-resize";
        resize.setAttribute("role", "separator");
        resize.setAttribute("aria-orientation", side === "bottom" ? "horizontal" : "vertical");
        dock.append(resize);
        this.root.append(rail, dock);
        this.dom.set(side, { rail, dock, resize });
        this.applySize(side);
        this.setupResize(side, resize);
    }
    setupResize(side, handle) {
        const onPointerDown = (event) => {
            event.preventDefault();
            const startX = event.clientX;
            const startY = event.clientY;
            const startSize = this.sizes[side];
            document.body.style.cursor = side === "bottom" ? "row-resize" : "col-resize";
            document.body.style.userSelect = "none";
            const onMove = (moveEvent) => {
                const delta = side === "left"
                    ? moveEvent.clientX - startX
                    : side === "right"
                        ? startX - moveEvent.clientX
                        : startY - moveEvent.clientY;
                const minimum = side === "bottom" ? this.options.minimumBottomSize : this.options.minimumSideSize;
                const maximum = side === "bottom" ? this.options.maximumBottomSize : this.options.maximumSideSize;
                this.sizes[side] = Math.max(minimum, Math.min(maximum, startSize + delta));
                this.applySize(side);
                this.updateLayout();
            };
            const onUp = () => {
                document.body.style.cursor = "";
                document.body.style.userSelect = "";
                document.removeEventListener("pointermove", onMove);
                document.removeEventListener("pointerup", onUp);
                document.removeEventListener("pointercancel", onUp);
            };
            document.addEventListener("pointermove", onMove);
            document.addEventListener("pointerup", onUp);
            document.addEventListener("pointercancel", onUp);
        };
        handle.addEventListener("pointerdown", onPointerDown);
        this.cleanupCallbacks.push(() => handle.removeEventListener("pointerdown", onPointerDown));
    }
    applySize(side) {
        const dock = this.requireSide(side).dock;
        if (side === "bottom")
            dock.style.height = `${this.sizes[side]}px`;
        else
            dock.style.width = `${this.sizes[side]}px`;
    }
    updateSide(side) {
        const sideDom = this.requireSide(side);
        const visible = [...this.entries.values()].some((entry) => entry.side === side && entry.state !== "hidden");
        sideDom.dock.hidden = !visible;
        sideDom.rail.classList.toggle("has-active-panel", visible);
        this.updateLayout();
    }
    updateLayout() {
        const insets = {
            left: this.sideInset("left"),
            right: this.sideInset("right"),
            bottom: this.sideInset("bottom"),
        };
        this.root.style.setProperty("--gdm-content-left", `${insets.left}px`);
        this.root.style.setProperty("--gdm-content-right", `${insets.right}px`);
        this.root.style.setProperty("--gdm-content-bottom", `${insets.bottom}px`);
        this.root.dispatchEvent(customEvent("geometer-demo-panel-layout-change", insets));
    }
    sideInset(side) {
        if (this.requireSide(side).dock.hidden)
            return 0;
        return this.sizes[side] + this.options.railSize;
    }
    syncCollapseButton(entry) {
        const collapsed = entry.state === "collapsed";
        entry.collapseButton.textContent = collapsed ? "+" : "−";
        entry.collapseButton.title = collapsed ? "Expand" : "Collapse";
        entry.collapseButton.setAttribute("aria-label", `${collapsed ? "Expand" : "Collapse"} ${entry.panel.title}`);
    }
    requireSide(side) {
        const value = this.dom.get(side);
        if (!value)
            throw new Error(`Panel side ${side} has not been initialized.`);
        return value;
    }
    assertActive() {
        if (this.destroyed)
            throw new Error("PanelManager has been destroyed.");
    }
}
