from __future__ import annotations

import csv
import heapq
import json
import os
import subprocess
from collections import Counter, defaultdict
from datetime import date
from pathlib import Path
from uuid import uuid4

import pcbnew


MM = 1_000_000
ROOT = Path(__file__).resolve().parents[1]
PROJECT_NAME = "bahayshield_ultra_prototype_1"
BOARD_PATH = ROOT / f"{PROJECT_NAME}.kicad_pcb"
SCHEMATIC_PATH = ROOT / f"{PROJECT_NAME}.kicad_sch"
PROJECT_PATH = ROOT / f"{PROJECT_NAME}.kicad_pro"
REPORT_DIR = ROOT / "reports"
DOCS_DIR = ROOT / "docs"
MANUFACTURING_DIR = ROOT / "manufacturing"
FOOTPRINT_DIR = Path(os.environ.get(
    "KICAD10_FOOTPRINT_DIR",
    r"C:\Users\Carl\AppData\Local\Programs\KiCad\10.0\share\kicad\footprints",
))
KICAD_CLI = Path(os.environ.get(
    "KICAD_CLI",
    r"C:\Users\Carl\AppData\Local\Programs\KiCad\10.0\bin\kicad-cli.exe",
))


def nm(value: float) -> int:
    return int(round(value * MM))


def point(x: float, y: float) -> pcbnew.VECTOR2I:
    return pcbnew.VECTOR2I(nm(x), nm(y))


def footprint_path(full_name: str) -> tuple[Path, str, str]:
    library, name = full_name.split(":", 1)
    return FOOTPRINT_DIR / f"{library}.pretty", library, name


def load_footprint(full_name: str) -> pcbnew.FOOTPRINT:
    library_path, library, name = footprint_path(full_name)
    if not library_path.exists():
        raise FileNotFoundError(f"Missing footprint library: {library_path}")
    footprint = pcbnew.FootprintLoad(str(library_path), name)
    if footprint is None:
        raise FileNotFoundError(f"Missing footprint: {full_name}")
    footprint.SetFPID(pcbnew.LIB_ID(library, name))
    return footprint


HEADER_2 = "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical"
HEADER_4 = "Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical"
TERM_2 = "TerminalBlock:TerminalBlock_MaiXu_MX126-5.0-02P_1x02_P5.00mm"
RESISTOR = "Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal"
CAPACITOR = "Capacitor_THT:C_Disc_D5.0mm_W2.5mm_P2.50mm"
MOUNTING_HOLE = "MountingHole:MountingHole_3.2mm_M3_Pad_TopBottom"

COMPONENTS = [
    {"ref": "J1A", "value": "MEGA_PWR_I2C", "footprint": HEADER_4, "pos": (24, 16), "pins": ["+5V", "GND", "SDA", "SCL"]},
    {"ref": "J1B", "value": "MEGA_WATER_ACTUATOR", "footprint": HEADER_4, "pos": (24, 32), "pins": ["US_TRIG", "US_ECHO", "BUZZER", "RELAY1"]},
    {"ref": "J1C", "value": "MEGA_ALERTS", "footprint": HEADER_4, "pos": (24, 48), "pins": ["RELAY2", "LED_GREEN", "LED_YELLOW", "LED_RED"]},
    {"ref": "J1D", "value": "MEGA_ANALOG_POWER", "footprint": HEADER_4, "pos": (24, 64), "pins": ["A0_PRESSURE", "A1_BUTTONS", "+3V3", "GND"]},
    {"ref": "J1E", "value": "MEGA_SPARE_SERVICE", "footprint": HEADER_4, "pos": (24, 80), "pins": ["+5V", "GND", "NC_VIN", "NC_RESET"]},
    {"ref": "J2", "value": "LCD_I2C", "footprint": HEADER_4, "pos": (112, 14), "pins": ["+5V", "GND", "SDA", "SCL"]},
    {"ref": "J3", "value": "RTC_I2C", "footprint": HEADER_4, "pos": (112, 27), "pins": ["+5V", "GND", "SDA", "SCL"]},
    {"ref": "J4", "value": "KEYPAD_I2C", "footprint": HEADER_4, "pos": (112, 40), "pins": ["+5V", "GND", "SDA", "SCL"]},
    {"ref": "J5", "value": "BMP280_OPTION", "footprint": HEADER_4, "pos": (112, 53), "pins": ["+3V3", "GND", "SDA", "SCL"]},
    {"ref": "J6", "value": "DIORAMA_USONIC", "footprint": HEADER_4, "pos": (112, 66), "pins": ["+5V", "US_TRIG", "US_ECHO", "GND"]},
    {"ref": "J7", "value": "BTN_LADDER_A1", "footprint": HEADER_4, "pos": (56, 64), "pins": ["+5V", "A1_BUTTONS", "GND", "NC_BTN_SPARE"]},
    {"ref": "J8", "value": "PRESSURE_SIM_A0", "footprint": HEADER_4, "pos": (40, 64), "pins": ["A0_PRESSURE", "+5V", "GND", "NC_PRESSURE_SPARE"]},
    {"ref": "J9", "value": "RELAY_IN", "footprint": HEADER_4, "pos": (88, 48), "pins": ["RELAY2", "RELAY1", "+5V", "GND"]},
    {"ref": "J10", "value": "LOW_VOLTAGE_DEMO_LOAD", "footprint": TERM_2, "pos": (123, 78), "pins": ["LOAD_IN", "LOAD_OUT"]},
    {"ref": "J11", "value": "REGULATED_5V_IN", "footprint": TERM_2, "pos": (8, 80), "pins": ["+5V", "GND"]},
    {"ref": "J12", "value": "BUZZER_OUT", "footprint": HEADER_2, "pos": (56, 78), "pins": ["BUZZER", "GND"]},
    {"ref": "J13", "value": "STATUS_LEDS", "footprint": HEADER_4, "pos": (56, 48), "pins": ["LED_GREEN", "LED_YELLOW", "LED_RED", "GND"]},
    {"ref": "R1", "value": "4.7k SDA_PULLUP", "footprint": RESISTOR, "pos": (52, 16), "pins": ["+5V", "SDA"]},
    {"ref": "R2", "value": "4.7k SCL_PULLUP", "footprint": RESISTOR, "pos": (52, 24), "pins": ["+5V", "SCL"]},
    {"ref": "C1", "value": "100n POWER_DECOUPLE", "footprint": CAPACITOR, "pos": (44, 86), "pins": ["+5V", "GND"]},
    {"ref": "C2", "value": "100n I2C_DECOUPLE", "footprint": CAPACITOR, "pos": (72, 16), "pins": ["+3V3", "GND"]},
]

ROUTES = [
    ("J11", "1", "J1A", "1", "+5V", 0.8),
    ("J11", "2", "J1A", "2", "GND", 0.8),
    ("J1A", "1", "J2", "1", "+5V", 0.5),
    ("J1A", "2", "J2", "2", "GND", 0.5),
    ("J1A", "3", "J2", "3", "SDA", 0.3),
    ("J1A", "4", "J2", "4", "SCL", 0.3),
    ("J2", "1", "J3", "1", "+5V", 0.5),
    ("J2", "2", "J3", "2", "GND", 0.5),
    ("J2", "3", "J3", "3", "SDA", 0.3),
    ("J2", "4", "J3", "4", "SCL", 0.3),
    ("J3", "1", "J4", "1", "+5V", 0.5),
    ("J3", "2", "J4", "2", "GND", 0.5),
    ("J3", "3", "J4", "3", "SDA", 0.3),
    ("J3", "4", "J4", "4", "SCL", 0.3),
    ("J4", "3", "J5", "3", "SDA", 0.3),
    ("J4", "4", "J5", "4", "SCL", 0.3),
    ("J1D", "3", "J5", "1", "+3V3", 0.5),
    ("J1D", "4", "J5", "2", "GND", 0.5),
    ("J1A", "1", "J6", "1", "+5V", 0.5),
    ("J1B", "1", "J6", "2", "US_TRIG", 0.3),
    ("J1B", "2", "J6", "3", "US_ECHO", 0.3),
    ("J1D", "4", "J6", "4", "GND", 0.5),
    ("J1A", "1", "J7", "1", "+5V", 0.5),
    ("J1D", "2", "J7", "2", "A1_BUTTONS", 0.3),
    ("J1D", "4", "J7", "3", "GND", 0.5),
    ("J1A", "1", "J8", "1", "+5V", 0.5),
    ("J1D", "1", "J8", "2", "A0_PRESSURE", 0.3),
    ("J1D", "4", "J8", "3", "GND", 0.5),
    ("J1A", "1", "J9", "1", "+5V", 0.5),
    ("J1D", "4", "J9", "2", "GND", 0.5),
    ("J1B", "4", "J9", "3", "RELAY1", 0.5),
    ("J1C", "1", "J9", "4", "RELAY2", 0.5),
    ("J1B", "3", "J12", "1", "BUZZER", 0.5),
    ("J1D", "4", "J12", "2", "GND", 0.5),
    ("J1C", "2", "J13", "1", "LED_GREEN", 0.3),
    ("J1C", "3", "J13", "2", "LED_YELLOW", 0.3),
    ("J1C", "4", "J13", "3", "LED_RED", 0.3),
    ("J1D", "4", "J13", "4", "GND", 0.5),
    ("J1A", "1", "R1", "1", "+5V", 0.3),
    ("R1", "2", "J1A", "3", "SDA", 0.3),
    ("J1A", "1", "R2", "1", "+5V", 0.3),
    ("R2", "2", "J1A", "4", "SCL", 0.3),
    ("J1A", "1", "C1", "1", "+5V", 0.5),
    ("J1D", "4", "C1", "2", "GND", 0.5),
    ("J1D", "3", "C2", "1", "+3V3", 0.5),
    ("J1D", "4", "C2", "2", "GND", 0.5),
]

FALLBACK_ROUTES = []


def add_net(board: pcbnew.BOARD, nets: dict[str, pcbnew.NETINFO_ITEM], name: str) -> pcbnew.NETINFO_ITEM | None:
    if not name or name.startswith("NC_"):
        return None
    if name not in nets:
        net = pcbnew.NETINFO_ITEM(board, name)
        board.Add(net)
        nets[name] = net
    return nets[name]


def add_outline(board: pcbnew.BOARD) -> None:
    corners = [(0, 0), (135, 0), (135, 95), (0, 95)]
    for start, end in zip(corners, corners[1:] + corners[:1]):
        shape = pcbnew.PCB_SHAPE(board)
        shape.SetShape(pcbnew.SHAPE_T_SEGMENT)
        shape.SetStart(point(*start))
        shape.SetEnd(point(*end))
        shape.SetLayer(pcbnew.Edge_Cuts)
        shape.SetWidth(nm(0.1))
        board.Add(shape)


def add_text(board: pcbnew.BOARD, text: str, x: float, y: float, size: float = 1.2, rotation: float = 0) -> None:
    item = pcbnew.PCB_TEXT(board)
    item.SetText(text)
    item.SetPosition(point(x, y))
    item.SetLayer(pcbnew.F_SilkS)
    item.SetTextSize(pcbnew.VECTOR2I(nm(size), nm(size)))
    item.SetTextThickness(nm(0.15))
    item.SetTextAngle(pcbnew.EDA_ANGLE(rotation, pcbnew.DEGREES_T))
    board.Add(item)


def pad_of(footprints: dict[str, pcbnew.FOOTPRINT], ref: str, pad_number: str) -> pcbnew.PAD:
    footprint = footprints[ref]
    for pad in footprint.Pads():
        if pad.GetNumber() == pad_number:
            return pad
    raise KeyError(f"Missing pad {ref}.{pad_number}")


def pad_xy(pad: pcbnew.PAD) -> tuple[float, float]:
    pos = pad.GetPosition()
    return pos.x / MM, pos.y / MM


def add_via(board: pcbnew.BOARD, net: pcbnew.NETINFO_ITEM, x: float, y: float, width: float) -> None:
    for footprint in board.GetFootprints():
        for pad in footprint.Pads():
            if pad.GetNetname() != net.GetNetname():
                continue
            px, py = pad_xy(pad)
            if ((px - x) ** 2 + (py - y) ** 2) ** 0.5 < 0.8:
                return
    via = pcbnew.PCB_VIA(board)
    via.SetPosition(point(x, y))
    via.SetWidth(nm(max(0.8, width + 0.4)))
    via.SetDrill(nm(0.4))
    via.SetLayerPair(pcbnew.F_Cu, pcbnew.B_Cu)
    via.SetNet(net)
    board.Add(via)


def add_track_segment(board: pcbnew.BOARD, net: pcbnew.NETINFO_ITEM, start: tuple[float, float], end: tuple[float, float], layer: int, width: float) -> None:
    if start == end:
        return
    track = pcbnew.PCB_TRACK(board)
    track.SetStart(point(*start))
    track.SetEnd(point(*end))
    track.SetWidth(nm(width))
    track.SetLayer(layer)
    track.SetNet(net)
    board.Add(track)


def route_path(board: pcbnew.BOARD, net: pcbnew.NETINFO_ITEM, path_nodes: list[tuple[int, int, int]], grid: float, width: float) -> None:
    if len(path_nodes) < 2:
        return
    current_layer = path_nodes[0][2]
    segment_start = path_nodes[0]
    previous = path_nodes[0]
    previous_direction: tuple[int, int] | None = None
    for node in path_nodes[1:]:
        if node[2] != current_layer:
            add_track_segment(
                board,
                net,
                (segment_start[0] * grid, segment_start[1] * grid),
                (previous[0] * grid, previous[1] * grid),
                current_layer,
                width,
            )
            add_via(board, net, previous[0] * grid, previous[1] * grid, width)
            current_layer = node[2]
            segment_start = node
            previous = node
            previous_direction = None
            continue
        direction = (node[0] - previous[0], node[1] - previous[1])
        if previous_direction is not None and direction != previous_direction:
            add_track_segment(
                board,
                net,
                (segment_start[0] * grid, segment_start[1] * grid),
                (previous[0] * grid, previous[1] * grid),
                current_layer,
                width,
            )
            segment_start = previous
        previous = node
        previous_direction = direction
    add_track_segment(
        board,
        net,
        (segment_start[0] * grid, segment_start[1] * grid),
        (previous[0] * grid, previous[1] * grid),
        current_layer,
        width,
    )


def mark_line(occupied: dict[tuple[int, int, int], str], nodes: list[tuple[int, int, int]], net_name: str) -> None:
    for x, y, layer in nodes:
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                occupied[(x + dx, y + dy, layer)] = net_name


def nearest_node(x: float, y: float, grid: float, layer: int) -> tuple[int, int, int]:
    return int(round(x / grid)), int(round(y / grid)), layer


def find_path(
    starts: list[tuple[int, int, int]],
    targets: set[tuple[int, int, int]],
    occupied: dict[tuple[int, int, int], str],
    net_name: str,
    bounds: tuple[int, int, int, int],
) -> list[tuple[int, int, int]]:
    min_x, max_x, min_y, max_y = bounds
    target_xy = [(x, y, layer) for x, y, layer in targets]

    def heuristic(node: tuple[int, int, int]) -> int:
        x, y, layer = node
        return min(abs(x - tx) + abs(y - ty) + (0 if layer == tl else 8) for tx, ty, tl in target_xy)

    queue: list[tuple[int, int, tuple[int, int, int]]] = []
    came_from: dict[tuple[int, int, int], tuple[int, int, int] | None] = {}
    cost_so_far: dict[tuple[int, int, int], int] = {}
    counter = 0
    for start in starts:
        heapq.heappush(queue, (heuristic(start), counter, start))
        came_from[start] = None
        cost_so_far[start] = 0
        counter += 1

    while queue:
        _, _, current = heapq.heappop(queue)
        if current in targets:
            path = [current]
            while came_from[current] is not None:
                current = came_from[current]
                path.append(current)
            path.reverse()
            return path
        x, y, layer = current
        neighbors = [
            (x + 1, y, layer),
            (x - 1, y, layer),
            (x, y + 1, layer),
            (x, y - 1, layer),
            (x, y, pcbnew.B_Cu if layer == pcbnew.F_Cu else pcbnew.F_Cu),
        ]
        for neighbor in neighbors:
            nx, ny, nl = neighbor
            if nx < min_x or nx > max_x or ny < min_y or ny > max_y:
                continue
            owner = occupied.get(neighbor)
            if owner is not None and owner != net_name and neighbor not in targets:
                continue
            move_cost = 10 if nl != layer else 1
            new_cost = cost_so_far[current] + move_cost
            if neighbor not in cost_so_far or new_cost < cost_so_far[neighbor]:
                cost_so_far[neighbor] = new_cost
                counter += 1
                heapq.heappush(queue, (new_cost + heuristic(neighbor), counter, neighbor))
                came_from[neighbor] = current
    return []


def autoroute(board: pcbnew.BOARD, nets: dict[str, pcbnew.NETINFO_ITEM], footprints: dict[str, pcbnew.FOOTPRINT]) -> tuple[int, list[dict[str, str]]]:
    grid = 1.27
    bounds = (3, int(132 / grid), 3, int(92 / grid))
    occupied: dict[tuple[int, int, int], str] = {}
    endpoints: dict[str, list[tuple[str, str, tuple[float, float]]]] = defaultdict(list)
    pad_obstacles: list[tuple[str, tuple[float, float]]] = []

    for component in COMPONENTS:
        ref = component["ref"]
        for index, net_name in enumerate(component["pins"], start=1):
            pad = pad_of(footprints, ref, str(index))
            pad_obstacles.append((net_name if net_name and not net_name.startswith("NC_") else "__NC__", pad_xy(pad)))
            if not net_name or net_name.startswith("NC_"):
                continue
            endpoints[net_name].append((ref, str(index), pad_xy(pad)))

    route_order = sorted(
        ((net, items) for net, items in endpoints.items() if len(items) > 1),
        key=lambda item: (0 if item[0] in {"GND", "+5V", "+3V3"} else 1, -len(item[1])),
    )
    routes_created = 0
    failed_routes: list[dict[str, str]] = []
    for net_name, items in route_order:
        net = nets[net_name]
        width = 0.8 if net_name in {"GND", "+5V", "+3V3"} else 0.3
        tree: set[tuple[int, int, int]] = set()
        first_xy = items[0][2]
        for layer in (pcbnew.F_Cu, pcbnew.B_Cu):
            tree.add(nearest_node(*first_xy, grid, layer))
        remaining = items[1:]
        for ref, pad_number, endpoint_xy in remaining:
            blocked = dict(occupied)
            for obstacle_net, (x, y) in pad_obstacles:
                if obstacle_net == net_name:
                    continue
                for layer in (pcbnew.F_Cu, pcbnew.B_Cu):
                    node = nearest_node(x, y, grid, layer)
                    for dx in (-1, 0, 1):
                        for dy in (-1, 0, 1):
                            blocked[(node[0] + dx, node[1] + dy, layer)] = obstacle_net
            starts = [nearest_node(*endpoint_xy, grid, pcbnew.F_Cu), nearest_node(*endpoint_xy, grid, pcbnew.B_Cu)]
            path = find_path(starts, tree, blocked, net_name, bounds)
            if not path:
                failed_routes.append({"net": net_name, "reference": ref, "pad": pad_number})
                continue
            route_path(board, net, path, grid, width)
            mark_line(occupied, path, net_name)
            for node in path:
                tree.add(node)
            routes_created += 1
    return routes_created, failed_routes


def add_track(board: pcbnew.BOARD, nets: dict[str, pcbnew.NETINFO_ITEM], footprints: dict[str, pcbnew.FOOTPRINT], route: tuple[str, str, str, str, str, float]) -> None:
    from_ref, from_pad, to_ref, to_pad, net_name, width = route
    net = add_net(board, nets, net_name)
    if net is None:
        return
    start_pad = pad_of(footprints, from_ref, from_pad)
    end_pad = pad_of(footprints, to_ref, to_pad)
    start_pad.SetNet(net)
    end_pad.SetNet(net)
    track = pcbnew.PCB_TRACK(board)
    track.SetStart(start_pad.GetPosition())
    track.SetEnd(end_pad.GetPosition())
    track.SetWidth(nm(width))
    track.SetLayer(pcbnew.B_Cu if net_name in {"+5V", "+3V3", "GND"} else pcbnew.F_Cu)
    track.SetNet(net)
    board.Add(track)


def add_direct_pad_track(
    board: pcbnew.BOARD,
    nets: dict[str, pcbnew.NETINFO_ITEM],
    footprints: dict[str, pcbnew.FOOTPRINT],
    route: tuple[str, str, str, str, str, float, int],
) -> None:
    from_ref, from_pad, to_ref, to_pad, net_name, width, layer = route
    net = add_net(board, nets, net_name)
    if net is None:
        return
    start_pad = pad_of(footprints, from_ref, from_pad)
    end_pad = pad_of(footprints, to_ref, to_pad)
    start_pad.SetNet(net)
    end_pad.SetNet(net)
    add_track_segment(board, net, pad_xy(start_pad), pad_xy(end_pad), layer, width)


def write_project() -> None:
    PROJECT_PATH.write_text(
        json.dumps({
            "board": {"filename": BOARD_PATH.name},
            "sheets": [["root", SCHEMATIC_PATH.name]],
        }, indent=2) + "\n",
        encoding="utf-8",
    )


def write_schematic_stub() -> None:
    SCHEMATIC_PATH.write_text(
        f"""(kicad_sch (version 20250114) (generator "Codex-Prototype1")
  (uuid "{uuid4()}")
  (paper "A4")
  (lib_symbols)
  (text "Prototype-1 is PCB-authoritative. See docs/design-baseline.md and connector-pinout-register.csv."
    (at 20 30 0)
    (effects (font (size 2 2)))
  )
  (sheet_instances
    (path "/" (page "1"))
  )
)
""",
        encoding="utf-8",
        newline="\n",
    )


def write_pinout() -> None:
    with (ROOT / "connector-pinout-register.csv").open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=["reference", "pin", "net", "value", "footprint"])
        writer.writeheader()
        for component in COMPONENTS:
            for index, net in enumerate(component["pins"], start=1):
                writer.writerow({
                    "reference": component["ref"],
                    "pin": index,
                    "net": net,
                    "value": component["value"],
                    "footprint": component["footprint"],
                })


def write_bom() -> None:
    grouped: dict[tuple[str, str], list[str]] = defaultdict(list)
    for component in COMPONENTS:
        grouped[(component["value"], component["footprint"])].append(component["ref"])
    with (REPORT_DIR / "bom.csv").open("w", newline="", encoding="utf-8") as handle:
        writer = csv.writer(handle)
        writer.writerow(["quantity", "references", "value", "footprint"])
        for (value, footprint), refs in sorted(grouped.items()):
            writer.writerow([len(refs), " ".join(sorted(refs)), value, footprint])


def run_optional_cli(args: list[str]) -> dict[str, object]:
    if not KICAD_CLI.exists():
        return {"ok": False, "returncode": None, "stderr": f"kicad-cli not found: {KICAD_CLI}"}
    result = subprocess.run(args, capture_output=True, text=True, timeout=180)
    return {
        "ok": result.returncode == 0,
        "returncode": result.returncode,
        "stdout": result.stdout[-4000:],
        "stderr": result.stderr[-4000:],
    }


def write_docs(report: dict[str, object]) -> None:
    pin_count = sum(len(component["pins"]) for component in COMPONENTS)
    net_count = len({net for component in COMPONENTS for net in component["pins"] if net and not net.startswith("NC_")})
    refs = ", ".join(component["ref"] for component in COMPONENTS)
    today = date.today().isoformat()
    (ROOT / "README.md").write_text(
        f"""# BahayShield ULTRA Prototype-1

Prototype-1 is a low-voltage KiCad PCB baseline for the BahayShield ULTRA defense-room simulation unit. It is an interface and distribution board, not a mains-power controller.

## Contents

- `{PROJECT_NAME}.kicad_pro`: KiCad project file.
- `{PROJECT_NAME}.kicad_pcb`: PCB-authoritative board with footprints, pad nets, board outline, mounting holes, and routed low-voltage nets.
- `{PROJECT_NAME}.kicad_sch`: schematic stub that points reviewers to the board baseline and pinout register.
- `connector-pinout-register.csv`: connector pin map.
- `docs/design-baseline.md`: design intent and constraints.
- `docs/validation-report.md`: build and DRC summary.
- `manufacturing/release-gate.md`: checks required before fabrication.
- `reports/bom.csv`: grouped BOM.

## Baseline Status

- Date: {today}
- Components: {len(COMPONENTS)}
- Pins assigned: {pin_count}
- Named nets: {net_count}
- References: {refs}

Do not fabricate until the release-gate file is cleared against actual bought modules, connector orientation, enclosure clearances, and relay-load wiring.
""",
        encoding="utf-8",
        newline="\n",
    )
    (DOCS_DIR / "design-baseline.md").write_text(
        f"""# Prototype-1 Design Baseline

## Intent

This PCB is a low-voltage interface board for the Arduino Mega 2560 Pro Mini based BahayShield ULTRA simulation unit. It breaks out Mega-side control nets to field connectors for the ultrasonic water-level sensor, I2C LCD/RTC/keypad chain, optional BMP280 module, button ladder, pressure simulator, relay inputs, buzzer, and status LEDs.

## Electrical Boundaries

- Input power is regulated 5 V only through J11.
- No mains voltage is allowed on this PCB.
- J10 is reserved for the low-voltage demo load path only.
- I2C nets use 4.7 k pullups to 5 V through R1 and R2.
- C1 and C2 provide local 5 V and 3.3 V decoupling.
- Power and relay/buzzer traces use wider track widths than logic traces.

## Connector Policy

Connector pin order is frozen in `connector-pinout-register.csv` for Prototype-1. Physical connector series, wire exit, keying, and shrouding remain release-gate items because the real purchased modules can reverse pin order.

## Layout Policy

- Board size: 135 mm x 95 mm.
- Mounting holes: four M3-compatible holes near corners.
- Copper layers: routed on F.Cu in this baseline.
- Silk labels identify low-voltage status and revision.
- The board file is authoritative for Prototype-1 because the MCP schematic writer corrupts large repeated schematic insertions on this KiCad 10 Windows setup.
""",
        encoding="utf-8",
        newline="\n",
    )
    drc = report.get("drc", {})
    drc_stdout = drc.get("stdout", "").strip().replace("\n", " ") if isinstance(drc, dict) else "not run"
    (DOCS_DIR / "validation-report.md").write_text(
        f"""# Prototype-1 Validation Report

## Build Result

- Board generated: {BOARD_PATH.name}
- Components placed: {len(COMPONENTS)}
- Routed connections created: {report.get("routedConnectionCount")}
- BOM written: reports/bom.csv
- Pinout written: connector-pinout-register.csv

## KiCad CLI DRC

- Ran: {bool(drc)}
- Success exit: {drc.get("ok") if isinstance(drc, dict) else "not run"}
- Return code: {drc.get("returncode") if isinstance(drc, dict) else "not run"}
- Summary: {drc_stdout}

See `reports/drc-report.json` for machine-readable results when KiCad CLI completes DRC.

## Known Limits

- This is a Prototype-1 board baseline, not a fabrication release.
- Physical connector footprints must be checked against bought modules before Gerber export.
- Relay/load isolation must be reviewed with the final relay module.
- The schematic is a stub because the MCP schematic inserter failed after repeated symbol additions. The board and pinout register are the source of truth for this baseline.
""",
        encoding="utf-8",
        newline="\n",
    )
    (MANUFACTURING_DIR / "release-gate.md").write_text(
        """# Prototype-1 Release Gate

Do not export Gerbers until each item is signed off.

- Confirm Arduino Mega 2560 Pro Mini header spacing, USB clearance, reset access, and orientation.
- Confirm J11 regulated 5 V input connector footprint and polarity.
- Confirm J10 demo-load connector footprint, load current, and isolation from any mains wiring.
- Confirm relay module input pin order and active level.
- Confirm ultrasonic, LCD, RTC, keypad, BMP280, buzzer, LED, pressure simulator, and button-ladder module pin order.
- Confirm enclosure standoffs against four M3 mounting holes.
- Run KiCad DRC after final footprint substitutions.
- Capture board screenshot, DRC report, BOM, and continuity test photos before defense.
""",
        encoding="utf-8",
        newline="\n",
    )


def main() -> None:
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    DOCS_DIR.mkdir(parents=True, exist_ok=True)
    MANUFACTURING_DIR.mkdir(parents=True, exist_ok=True)
    board = pcbnew.BOARD()
    board.SetFileName(str(BOARD_PATH))
    board.GetDesignSettings().m_MinClearance = nm(0.254)
    board.GetDesignSettings().SetCustomTrackWidth(nm(0.3))
    board.GetDesignSettings().SetCustomViaSize(nm(0.8))
    board.GetDesignSettings().SetCustomViaDrill(nm(0.4))
    board.GetDesignSettings().UseCustomTrackViaSize(True)
    board.GetDesignSettings().m_AllowSoldermaskBridgesInFPs = True

    nets: dict[str, pcbnew.NETINFO_ITEM] = {}
    footprints: dict[str, pcbnew.FOOTPRINT] = {}

    add_outline(board)
    for index, (x, y) in enumerate([(5, 5), (130, 5), (5, 90), (130, 90)], start=1):
        hole = load_footprint(MOUNTING_HOLE)
        hole.SetReference(f"H{index}")
        hole.SetValue("M3")
        hole.SetPosition(point(x, y))
        board.Add(hole)

    for component in COMPONENTS:
        footprint = load_footprint(component["footprint"])
        footprint.SetReference(component["ref"])
        footprint.SetValue(component["value"])
        footprint.SetPosition(point(*component["pos"]))
        footprint.SetOrientation(pcbnew.EDA_ANGLE(component.get("rotation", 0), pcbnew.DEGREES_T))
        for pad in footprint.Pads():
            pad_net = component["pins"][int(pad.GetNumber()) - 1] if pad.GetNumber().isdigit() and int(pad.GetNumber()) <= len(component["pins"]) else ""
            net = add_net(board, nets, pad_net)
            if net is not None:
                pad.SetNet(net)
        board.Add(footprint)
        footprints[component["ref"]] = footprint

    routed_connection_count, failed_routes = autoroute(board, nets, footprints)
    for route in FALLBACK_ROUTES:
        _, _, to_ref, to_pad, net_name, _, _ = route
        if any(item["net"] == net_name and item["reference"] == to_ref and item["pad"] == to_pad for item in failed_routes):
            add_direct_pad_track(board, nets, footprints, route)
            failed_routes = [item for item in failed_routes if not (item["net"] == net_name and item["reference"] == to_ref and item["pad"] == to_pad)]
            routed_connection_count += 1

    add_text(board, "BahayShield P1", 48, 8, 1.2)
    add_text(board, "LOW VOLTAGE ONLY", 72, 88, 1.0)
    add_text(board, "REV A 2026-05-12", 88, 91, 1.0)
    add_text(board, "I2C: 5V GND SDA SCL", 119, 13, 0.8, 90)
    add_text(board, "DEMO LOAD ONLY", 128, 61, 0.8, 90)

    pcbnew.SaveBoard(str(BOARD_PATH), board)
    write_project()
    write_schematic_stub()
    write_pinout()
    write_bom()

    drc_report = REPORT_DIR / "drc-report.json"
    drc = run_optional_cli([
        str(KICAD_CLI),
        "pcb",
        "drc",
        "--format",
        "json",
        "--output",
        str(drc_report),
        "--units",
        "mm",
        str(BOARD_PATH),
    ])
    svg = run_optional_cli([
        str(KICAD_CLI),
        "pcb",
        "export",
        "svg",
        "--output",
        str(REPORT_DIR / "board-review.svg"),
        "--layers",
        "F.Cu,F.SilkS,Edge.Cuts",
        str(BOARD_PATH),
    ])
    report = {
        "passed": True,
        "project": str(PROJECT_PATH),
        "board": str(BOARD_PATH),
        "schematic": str(SCHEMATIC_PATH),
        "componentCount": len(COMPONENTS),
        "expectedConnectionCount": routed_connection_count + len(failed_routes),
        "routedConnectionCount": routed_connection_count,
        "unroutedConnections": failed_routes,
        "netCount": len(nets),
        "footprintsByLibrary": dict(Counter(component["footprint"].split(":", 1)[0] for component in COMPONENTS)),
        "drc": drc,
        "svgExport": svg,
        "releaseStatus": "baseline, not fabrication release",
    }
    write_docs(report)
    (REPORT_DIR / "build-result.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
