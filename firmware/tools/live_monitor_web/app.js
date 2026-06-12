// AODV-EN Live Mesh Monitor - frontend
// WebSocket client + Cytoscape.js animated topology

(() => {
    "use strict";

    // ============================================================
    // State
    // ============================================================
    const state = {
        nodes: new Map(), // mac -> node dict
        events: [],
        ackCount: 0,
        deliveredCount: 0,
        ws: null,
        reconnectTimer: null,
    };

    // ============================================================
    // Helpers
    // ============================================================
    function macShort(mac) {
        if (!mac) return "—";
        return mac.slice(-5);
    }

    function nodeLabel(n) {
        if (!n) return "?";
        return n.alias || macShort(n.mac);
    }

    function fmtTime(tsMs) {
        const d = new Date(tsMs);
        return d.toTimeString().slice(0, 8);
    }

    function clamp(v, lo, hi) {
        return Math.max(lo, Math.min(hi, v));
    }

    // ============================================================
    // Cytoscape setup
    // ============================================================
    const cy = cytoscape({
        container: document.getElementById("cy"),
        style: [
            {
                selector: "node",
                style: {
                    width: 56,
                    height: 56,
                    "background-color": "#ffffff",
                    "border-width": 3,
                    "border-color": "#94a3b8",
                    label: "data(label)",
                    color: "#1a202c",
                    "font-size": 13,
                    "font-weight": 600,
                    "text-valign": "bottom",
                    "text-margin-y": 8,
                    "text-wrap": "wrap",
                    "text-max-width": 130,
                    "transition-property":
                        "border-color, background-color, border-width, opacity",
                    "transition-duration": "260ms",
                },
            },
            {
                selector: "node.role-gateway",
                style: {
                    "border-color": "#2563eb",
                    "background-color": "#eff6ff",
                },
            },
            {
                selector: "node.role-relay",
                style: {
                    "border-color": "#7c3aed",
                    "background-color": "#f5f3ff",
                },
            },
            {
                selector: "node.role-leaf",
                style: {
                    "border-color": "#16a34a",
                    "background-color": "#f0fdf4",
                },
            },
            {
                selector: "node.online",
                style: {
                    opacity: 1,
                },
            },
            {
                selector: "node.offline",
                style: {
                    opacity: 0.45,
                    "border-color": "#94a3b8",
                    "border-style": "dashed",
                },
            },
            {
                selector: "node.flash",
                style: {
                    "border-color": "#dc2626",
                    "border-width": 6,
                },
            },
            {
                selector: "node.pulse",
                style: {
                    "border-color": "#16a34a",
                    "border-width": 6,
                },
            },
            {
                selector: "edge",
                style: {
                    width: 2.5,
                    "line-color": "#6366f1",
                    "target-arrow-color": "#6366f1",
                    "target-arrow-shape": "triangle",
                    "arrow-scale": 1.2,
                    "curve-style": "bezier",
                    label: "data(label)",
                    "font-size": 10.5,
                    color: "#475569",
                    "text-background-color": "#ffffff",
                    "text-background-opacity": 0.85,
                    "text-background-padding": 3,
                    "text-background-shape": "round-rectangle",
                    "text-rotation": "autorotate",
                    "transition-property": "line-color, target-arrow-color, width, opacity",
                    "transition-duration": "260ms",
                },
            },
            {
                selector: "edge.state-valid",
                style: {
                    "line-color": "#6366f1",
                    "target-arrow-color": "#6366f1",
                },
            },
            {
                selector: "edge.state-reverse",
                style: {
                    "line-color": "#d97706",
                    "target-arrow-color": "#d97706",
                    "line-style": "dashed",
                },
            },
            {
                selector: "edge.state-invalid",
                style: {
                    "line-color": "#cbd5e0",
                    "target-arrow-color": "#cbd5e0",
                    "line-style": "dotted",
                },
            },
            {
                selector: "edge.flash",
                style: {
                    "line-color": "#dc2626",
                    "target-arrow-color": "#dc2626",
                    width: 4,
                },
            },
            {
                selector: "edge.pulse",
                style: {
                    "line-color": "#16a34a",
                    "target-arrow-color": "#16a34a",
                    width: 5,
                },
            },
        ],
        layout: { name: "preset" },
        wheelSensitivity: 0.2,
        minZoom: 0.4,
        maxZoom: 2.5,
    });

    function relayout() {
        const layout = cy.layout({
            name: "breadthfirst",
            directed: true,
            padding: 30,
            spacingFactor: 1.4,
            animate: true,
            animationDuration: 400,
            animationEasing: "ease-out",
        });
        layout.run();
    }

    document.getElementById("btn-relayout").addEventListener("click", relayout);
    document.getElementById("btn-fit").addEventListener("click", () => {
        cy.animate({ fit: { padding: 40 }, duration: 400, easing: "ease-out" });
    });

    // Tooltip
    const tooltip = document.getElementById("node-tooltip");
    cy.on("mouseover", "node", (evt) => {
        const n = state.nodes.get(evt.target.id());
        if (!n) return;
        const lines = [
            `<b>${nodeLabel(n)}</b>`,
            `mac: ${n.mac}`,
            `online: ${n.online ? "sim" : "nao"}`,
            `routes: ${n.stats.routes_count}  neighbors: ${n.stats.neighbors_count}`,
            `tx: ${n.stats.tx}  rx: ${n.stats.rx}  delivered: ${n.stats.delivered}`,
        ];
        tooltip.innerHTML = lines.join("<br>");
        tooltip.classList.remove("hidden");
    });
    cy.on("mouseout", "node", () => tooltip.classList.add("hidden"));
    cy.on("mousemove", (evt) => {
        const e = evt.originalEvent;
        if (!e) return;
        tooltip.style.left = e.pageX + 14 + "px";
        tooltip.style.top = e.pageY + 14 + "px";
    });

    // ============================================================
    // Render: nodes, edges, sidebar
    // ============================================================
    function inferRole(n) {
        if (!n.online) return "offline";
        const stats = n.stats || {};
        // gateway: more delivered/tx than rx forwarding implies origin
        // simple heuristic for v1: if alias contains GW use gateway
        if (n.alias && /GW|GATEWAY/i.test(n.alias)) return "gateway";
        // routes count > 1 + neighbors count >= 2 = relay
        if ((stats.neighbors_count || 0) >= 2 && (stats.routes_count || 0) >= 2) {
            return "relay";
        }
        // default: leaf
        return "leaf";
    }

    function syncCyNodes() {
        // add or update nodes
        for (const [mac, n] of state.nodes) {
            let cn = cy.getElementById(mac);
            if (cn.empty()) {
                cn = cy.add({
                    group: "nodes",
                    data: { id: mac, label: nodeLabel(n) },
                });
            } else {
                cn.data("label", nodeLabel(n));
            }
            const role = inferRole(n);
            cn.removeClass("role-gateway role-relay role-leaf online offline");
            cn.addClass(n.online ? "online" : "offline");
            if (role !== "offline") cn.addClass(`role-${role}`);
        }
        // remove ghost nodes that are no longer present
        cy.nodes().forEach((cn) => {
            if (!state.nodes.has(cn.id())) {
                cn.remove();
            }
        });
    }

    function syncCyEdges() {
        // build target set from all node routes
        const wanted = new Map(); // edge id -> { source, target, label, classes, hops, state }
        for (const [mac, n] of state.nodes) {
            for (const r of n.routes || []) {
                if (!r.dest || !r.via) continue;
                if (r.dest === mac) continue;
                if (!state.nodes.has(r.via)) continue; // skip if next hop unknown
                const edgeId = `${mac}__${r.via}`;
                const stateClass =
                    r.state === 2
                        ? "state-valid"
                        : r.state === 1
                            ? "state-reverse"
                            : "state-invalid";
                // Aggregate: prefer the highest-state, fewest-hops summary for that next-hop
                const cur = wanted.get(edgeId);
                const incomingScore = r.state * 100 - r.hops;
                if (!cur || incomingScore > cur.score) {
                    wanted.set(edgeId, {
                        source: mac,
                        target: r.via,
                        label: `via→ hops≥${r.hops}`,
                        classes: stateClass,
                        score: incomingScore,
                        destinations: new Set([r.dest]),
                    });
                } else {
                    cur.destinations.add(r.dest);
                }
            }
        }
        // update labels with destination count
        for (const [, edge] of wanted) {
            const destCount = edge.destinations.size;
            edge.label = `${destCount} dest`;
        }

        // add/update edges
        for (const [edgeId, edge] of wanted) {
            let ce = cy.getElementById(edgeId);
            if (ce.empty()) {
                ce = cy.add({
                    group: "edges",
                    data: {
                        id: edgeId,
                        source: edge.source,
                        target: edge.target,
                        label: edge.label,
                    },
                });
            } else {
                ce.data("label", edge.label);
            }
            ce.removeClass("state-valid state-reverse state-invalid");
            ce.addClass(edge.classes);
        }
        // remove edges no longer present
        cy.edges().forEach((ce) => {
            if (!wanted.has(ce.id()) && !ce.hasClass("transient")) {
                ce.remove();
            }
        });
    }

    // ============================================================
    // Animations
    // ============================================================
    function flashNode(mac, ms = 700) {
        const cn = cy.getElementById(mac);
        if (cn.empty()) return;
        cn.addClass("flash");
        setTimeout(() => cn.removeClass("flash"), ms);
    }

    function pulseNode(mac, ms = 900) {
        const cn = cy.getElementById(mac);
        if (cn.empty()) return;
        cn.addClass("pulse");
        setTimeout(() => cn.removeClass("pulse"), ms);
    }

    function flashEdge(srcMac, dstMac, ms = 700) {
        const ce = cy.getElementById(`${srcMac}__${dstMac}`);
        if (ce.empty()) return;
        ce.addClass("flash");
        setTimeout(() => ce.removeClass("flash"), ms);
    }

    /**
     * Compute path src -> dst by chaining route.via through known nodes.
     * Returns array of macs starting with src and ending with dst, or null.
     */
    function computePath(srcMac, dstMac, maxHops = 16) {
        const path = [srcMac];
        let cur = srcMac;
        const visited = new Set();
        for (let i = 0; i < maxHops; i++) {
            if (cur === dstMac) return path;
            if (visited.has(cur)) return null;
            visited.add(cur);
            const n = state.nodes.get(cur);
            if (!n) return null;
            const route = (n.routes || []).find(
                (r) => r.dest === dstMac && r.state === 2
            );
            if (!route) return null;
            path.push(route.via);
            cur = route.via;
        }
        return cur === dstMac ? path : null;
    }

    function pulsePath(macs, color = "#16a34a", stepMs = 360) {
        if (!macs || macs.length < 2) return;
        for (let i = 0; i < macs.length - 1; i++) {
            const a = macs[i];
            const b = macs[i + 1];
            setTimeout(() => {
                const ce = cy.getElementById(`${a}__${b}`);
                if (!ce.empty()) {
                    ce.addClass("pulse");
                    setTimeout(() => ce.removeClass("pulse"), stepMs + 200);
                }
                pulseNode(b, stepMs + 200);
            }, i * stepMs);
        }
    }

    // ============================================================
    // Sidebar render
    // ============================================================
    function renderSidebar() {
        // metrics
        let online = 0;
        let total = 0;
        let maxHops = 0;
        let validRoutes = 0;
        let totalRoutes = 0;
        let totalDelivered = 0;

        for (const n of state.nodes.values()) {
            total++;
            if (n.online) online++;
            for (const r of n.routes || []) {
                totalRoutes++;
                if (r.state === 2) validRoutes++;
                if (r.hops > maxHops) maxHops = r.hops;
            }
            totalDelivered += n.stats.delivered || 0;
        }

        document.getElementById("m-online").textContent = online;
        document.getElementById("m-known").textContent = total;
        document.getElementById("m-hops").textContent =
            maxHops > 0 ? maxHops : "—";
        document.getElementById("m-routes").textContent =
            totalRoutes > 0 ? `${validRoutes}/${totalRoutes}` : "—";
        document.getElementById("m-delivered").textContent = totalDelivered;
        document.getElementById("m-ack").textContent = state.ackCount;

        // nodes list
        const list = document.getElementById("nodes-list");
        if (state.nodes.size === 0) {
            list.innerHTML = '<div class="empty">Nenhum no detectado ainda.</div>';
        } else {
            const rows = [];
            const sorted = Array.from(state.nodes.values()).sort((a, b) => {
                const ka = a.alias || a.mac;
                const kb = b.alias || b.mac;
                return ka.localeCompare(kb);
            });
            for (const n of sorted) {
                const role = inferRole(n);
                rows.push(`
                    <div class="node-row ${n.online ? "" : "offline"}" data-mac="${n.mac}">
                        <div class="node-row-left">
                            <span class="node-bullet role-${role}"></span>
                            <div class="node-info">
                                <div class="node-name">${nodeLabel(n)}</div>
                                <div class="node-mac">${n.mac}</div>
                            </div>
                        </div>
                        <div class="node-stats">
                            tx ${n.stats.tx}  rx ${n.stats.rx}<br>
                            ${n.stats.routes_count}r · ${n.stats.neighbors_count}n
                        </div>
                    </div>
                `);
            }
            list.innerHTML = rows.join("");
            list.querySelectorAll(".node-row").forEach((row) => {
                row.addEventListener("click", () => {
                    const mac = row.dataset.mac;
                    const cn = cy.getElementById(mac);
                    if (!cn.empty()) {
                        cy.animate({
                            center: { eles: cn },
                            duration: 300,
                        });
                        pulseNode(mac, 1200);
                    }
                });
            });
        }
    }

    function appendEvent(ev) {
        state.events.push({ ...ev, _localTs: Date.now() });
        if (state.events.length > 60) state.events.shift();
        renderEvents();
    }

    function renderEvents() {
        const list = document.getElementById("events-list");
        if (state.events.length === 0) {
            list.innerHTML = '<div class="empty">Sem eventos.</div>';
            return;
        }
        const html = state.events
            .slice()
            .reverse()
            .map((ev) => {
                const time = fmtTime(ev._localTs);
                const macAlias = aliasOrMac(ev.mac);
                let detail = "";
                switch (ev.type) {
                    case "ack":
                        detail = `${macAlias} ← ${aliasOrMac(ev.from_mac)} seq=${ev.seq}`;
                        break;
                    case "deliver":
                        detail = `${macAlias} ← ${aliasOrMac(ev.from_mac)}`;
                        break;
                    case "send_fail":
                        detail = `${macAlias} → ${aliasOrMac(ev.to_mac)}`;
                        break;
                    case "data_status":
                        detail = `${macAlias} status=${ev.status}`;
                        break;
                    case "invalidated":
                        detail = `${macAlias} via ${aliasOrMac(ev.via)} (${ev.count} rotas)`;
                        break;
                    case "queued_route":
                    case "queued_discovery":
                        detail = macAlias;
                        break;
                    case "node_seen":
                        detail = `${ev.alias || macShort(ev.mac)} canal=${ev.channel}`;
                        break;
                    case "stats":
                        detail = `${macAlias} tx=${ev.stats.tx} rx=${ev.stats.rx} d=${ev.stats.delivered}`;
                        break;
                    case "routes":
                        detail = `${macAlias} (${ev.routes.length} rotas)`;
                        break;
                    default:
                        detail = JSON.stringify(ev).slice(0, 80);
                }
                return `<div class="event-line ev-${ev.type}">
                    <span class="event-time">${time}</span><span class="event-type">${ev.type.padEnd(9)}</span>${detail}
                </div>`;
            })
            .join("");
        list.innerHTML = html;
    }

    function aliasOrMac(mac) {
        if (!mac) return "?";
        const n = state.nodes.get(mac);
        if (n && n.alias) return n.alias;
        return macShort(mac);
    }

    // ============================================================
    // Event apply
    // ============================================================
    function touchNodeAlive(mac) {
        if (!mac) return;
        const n = state.nodes.get(mac);
        if (!n) return;
        n.online = true;
        n.last_seen = Date.now() / 1000;
    }

    function applyEvent(ev) {
        switch (ev.type) {
            case "snapshot": {
                state.nodes.clear();
                for (const n of ev.nodes || []) {
                    state.nodes.set(n.mac, n);
                }
                state.events = (ev.events || []).map((e) => ({
                    ...e,
                    _localTs: Date.now(),
                }));
                state.ackCount = state.events.filter((e) => e.type === "ack").length;
                renderAll();
                relayout();
                return;
            }
            case "node_seen": {
                let n = state.nodes.get(ev.mac);
                if (!n) {
                    n = {
                        mac: ev.mac,
                        alias: null,
                        channel: null,
                        network_id: null,
                        routes: [],
                        stats: {
                            routes_count: 0,
                            neighbors_count: 0,
                            tx: 0,
                            rx: 0,
                            delivered: 0,
                        },
                        last_seen: 0,
                        online: true,
                    };
                    state.nodes.set(ev.mac, n);
                }
                if (ev.alias) n.alias = ev.alias;
                if (ev.channel != null) n.channel = ev.channel;
                if (ev.network_id) n.network_id = ev.network_id;
                n.online = true;
                n.last_seen = Date.now() / 1000;
                appendEvent(ev);
                syncCyNodes();
                pulseNode(ev.mac, 900);
                renderSidebar();
                // relayout when topology grows
                if (cy.nodes().length <= 6) relayout();
                return;
            }
            case "stats": {
                let n = state.nodes.get(ev.mac);
                if (!n) return;
                n.stats = { ...n.stats, ...ev.stats };
                n.online = true;
                n.last_seen = Date.now() / 1000;
                renderSidebar();
                return;
            }
            case "routes": {
                let n = state.nodes.get(ev.mac);
                if (!n) return;
                n.routes = ev.routes;
                n.online = true;
                n.last_seen = Date.now() / 1000;
                const now = Date.now() / 1000;
                // ensure ghost nodes for unknown destinations
                for (const r of ev.routes) {
                    if (!state.nodes.has(r.dest)) {
                        state.nodes.set(r.dest, {
                            mac: r.dest,
                            alias: null,
                            channel: null,
                            network_id: null,
                            routes: [],
                            stats: {
                                routes_count: 0,
                                neighbors_count: 0,
                                tx: 0,
                                rx: 0,
                                delivered: 0,
                            },
                            last_seen: 0,
                            online: false,
                        });
                    }
                    if (!state.nodes.has(r.via)) {
                        state.nodes.set(r.via, {
                            mac: r.via,
                            alias: null,
                            channel: null,
                            network_id: null,
                            routes: [],
                            stats: {
                                routes_count: 0,
                                neighbors_count: 0,
                                tx: 0,
                                rx: 0,
                                delivered: 0,
                            },
                            last_seen: 0,
                            online: false,
                        });
                    }
                    // Se a rota e VALID (state == 2), considera dest e via online.
                    // Isso permite que ghost nodes reflitam o que o protocolo
                    // ja sabe: o caminho esta vivo, mesmo sem serial proprio.
                    if (r.state === 2) {
                        const dest = state.nodes.get(r.dest);
                        if (dest) {
                            dest.online = true;
                            dest.last_seen = now;
                        }
                        const via = state.nodes.get(r.via);
                        if (via) {
                            via.online = true;
                            via.last_seen = now;
                        }
                    }
                }
                syncCyNodes();
                syncCyEdges();
                renderSidebar();
                return;
            }
            case "ack": {
                state.ackCount++;
                touchNodeAlive(ev.mac);
                appendEvent(ev);
                pulseNode(ev.mac, 900);
                pulseNode(ev.from_mac, 900);
                const path = computePath(ev.mac, ev.from_mac);
                if (path) pulsePath(path);
                renderSidebar();
                showAlertOnAckGap();
                return;
            }
            case "deliver": {
                state.deliveredCount++;
                touchNodeAlive(ev.mac);
                appendEvent(ev);
                pulseNode(ev.mac, 900);
                renderSidebar();
                return;
            }
            case "send_fail": {
                appendEvent(ev);
                flashEdge(ev.mac, ev.to_mac);
                flashNode(ev.to_mac, 700);
                renderSidebar();
                triggerAlert(
                    "Falha de enlace",
                    `${aliasOrMac(ev.mac)} nao conseguiu transmitir para ${aliasOrMac(ev.to_mac)}.`
                );
                return;
            }
            case "data_status": {
                appendEvent(ev);
                if (ev.status === -2) {
                    flashNode(ev.mac, 700);
                    triggerAlert(
                        "Fila de DATA cheia",
                        `${aliasOrMac(ev.mac)} retornou status=-2 (AODV_EN_ERR_FULL).`
                    );
                }
                renderSidebar();
                return;
            }
            case "invalidated": {
                appendEvent(ev);
                flashEdge(ev.mac, ev.via);
                flashNode(ev.mac, 700);
                renderSidebar();
                triggerAlert(
                    "Rotas invalidadas",
                    `${aliasOrMac(ev.mac)} invalidou ${ev.count} rota(s) via ${aliasOrMac(ev.via)} apos falhas.`
                );
                return;
            }
            case "queued_discovery":
            case "queued_route": {
                touchNodeAlive(ev.mac);
                appendEvent(ev);
                pulseNode(ev.mac, 700);
                renderSidebar();
                return;
            }
            default:
                appendEvent(ev);
                renderSidebar();
                return;
        }
    }

    function renderAll() {
        syncCyNodes();
        syncCyEdges();
        renderSidebar();
        renderEvents();
    }

    // ============================================================
    // Alert card
    // ============================================================
    let alertTimer = null;
    function triggerAlert(title, msg) {
        const card = document.getElementById("alert-card");
        document.getElementById("alert-title").textContent = title;
        document.getElementById("alert-msg").textContent = msg;
        card.classList.remove("hidden");
        if (alertTimer) clearTimeout(alertTimer);
        alertTimer = setTimeout(() => card.classList.add("hidden"), 6000);
    }

    function showAlertOnAckGap() {
        // simple heuristic: if ack count grows but a previous send_fail happened recently, show it
        // for now just dismiss the alert when ACKs flow
    }

    // ============================================================
    // WebSocket
    // ============================================================
    function setConnState(label, dotClass) {
        const conn = document.getElementById("connection");
        conn.querySelector(".dot").className = `dot ${dotClass}`;
        conn.querySelector(".conn-text").textContent = label;
    }

    function connect() {
        const proto = window.location.protocol === "https:" ? "wss:" : "ws:";
        const url = `${proto}//${window.location.host}/ws`;
        try {
            state.ws = new WebSocket(url);
        } catch (e) {
            setConnState("erro", "dot-offline");
            scheduleReconnect();
            return;
        }
        setConnState("conectando...", "dot-pending");

        state.ws.onopen = () => {
            setConnState("conectado", "dot-online");
        };

        state.ws.onmessage = (ev) => {
            try {
                const msg = JSON.parse(ev.data);
                applyEvent(msg);
            } catch (e) {
                console.warn("ignored malformed ws message", e);
            }
        };

        state.ws.onclose = () => {
            setConnState("desconectado", "dot-offline");
            scheduleReconnect();
        };

        state.ws.onerror = () => {
            setConnState("erro", "dot-offline");
        };
    }

    function scheduleReconnect() {
        if (state.reconnectTimer) return;
        state.reconnectTimer = setTimeout(() => {
            state.reconnectTimer = null;
            connect();
        }, 2000);
    }

    // ============================================================
    // Boot
    // ============================================================
    renderAll();
    connect();

    // periodic re-evaluation of online status (in case stats stop arriving)
    setInterval(() => {
        const now = Date.now() / 1000;
        let changed = false;
        for (const n of state.nodes.values()) {
            const wasOnline = n.online;
            const stillOnline =
                n.last_seen && now - n.last_seen < 30 ? true : false;
            if (wasOnline !== stillOnline) {
                n.online = stillOnline;
                changed = true;
            }
        }
        if (changed) {
            syncCyNodes();
            renderSidebar();
        }
    }, 5000);
})();
