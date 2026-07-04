from __future__ import annotations

import math
import struct
from pathlib import Path


Triangle = tuple[tuple[float, float, float], tuple[float, float, float], tuple[float, float, float]]

PANEL_W = 280.0
PANEL_D = 200.0
PANEL_T = 3.2
BASE_H = 68.0
WALL = 3.0
FLOOR_T = 3.0

PCB_SIZE = 125.0
PCB_X = 18.0
PCB_Y = 42.0
PCB_STANDOFF_H = 10.0
PCB_HOLES = (
    (6.00, 6.00),      # H1 from Rev A manual KiCad release
    (120.30, 5.30),    # H2
    (6.40, 118.60),    # H3
    (119.20, 119.10),  # H4
)
DC_JACK_CENTER_X = 253.0
DC_JACK_CENTER_Z = 34.0
DC_JACK_HOLE_D = 12.4


def normal(triangle: Triangle) -> tuple[float, float, float]:
    a, b, c = triangle
    ux, uy, uz = b[0] - a[0], b[1] - a[1], b[2] - a[2]
    vx, vy, vz = c[0] - a[0], c[1] - a[1], c[2] - a[2]
    nx = uy * vz - uz * vy
    ny = uz * vx - ux * vz
    nz = ux * vy - uy * vx
    length = math.sqrt(nx * nx + ny * ny + nz * nz)
    if length == 0:
        return 0.0, 0.0, 0.0
    return nx / length, ny / length, nz / length


def add_quad(triangles: list[Triangle], a, b, c, d) -> None:
    triangles.append((a, b, c))
    triangles.append((a, c, d))


def add_box(triangles: list[Triangle], x0: float, y0: float, z0: float, x1: float, y1: float, z1: float) -> None:
    p000 = (x0, y0, z0)
    p100 = (x1, y0, z0)
    p110 = (x1, y1, z0)
    p010 = (x0, y1, z0)
    p001 = (x0, y0, z1)
    p101 = (x1, y0, z1)
    p111 = (x1, y1, z1)
    p011 = (x0, y1, z1)
    add_quad(triangles, p000, p100, p110, p010)
    add_quad(triangles, p001, p011, p111, p101)
    add_quad(triangles, p000, p001, p101, p100)
    add_quad(triangles, p100, p101, p111, p110)
    add_quad(triangles, p110, p111, p011, p010)
    add_quad(triangles, p010, p011, p001, p000)


def add_cylinder_shell(
    triangles: list[Triangle],
    cx: float,
    cy: float,
    z0: float,
    z1: float,
    outer_r: float,
    inner_r: float,
    segments: int = 48,
) -> None:
    for index in range(segments):
        a0 = 2 * math.pi * index / segments
        a1 = 2 * math.pi * (index + 1) / segments
        outer0 = (cx + outer_r * math.cos(a0), cy + outer_r * math.sin(a0))
        outer1 = (cx + outer_r * math.cos(a1), cy + outer_r * math.sin(a1))
        inner0 = (cx + inner_r * math.cos(a0), cy + inner_r * math.sin(a0))
        inner1 = (cx + inner_r * math.cos(a1), cy + inner_r * math.sin(a1))
        add_quad(triangles, (outer0[0], outer0[1], z0), (outer1[0], outer1[1], z0), (outer1[0], outer1[1], z1), (outer0[0], outer0[1], z1))
        add_quad(triangles, (inner1[0], inner1[1], z0), (inner0[0], inner0[1], z0), (inner0[0], inner0[1], z1), (inner1[0], inner1[1], z1))
        add_quad(triangles, (outer0[0], outer0[1], z1), (outer1[0], outer1[1], z1), (inner1[0], inner1[1], z1), (inner0[0], inner0[1], z1))
        add_quad(triangles, (outer1[0], outer1[1], z0), (outer0[0], outer0[1], z0), (inner0[0], inner0[1], z0), (inner1[0], inner1[1], z0))


def write_stl(path: Path, name: str, triangles: list[Triangle]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    header = f"BahayShield ULTRA {name}".encode("ascii")[:80].ljust(80, b" ")
    with path.open("wb") as stl_file:
        stl_file.write(header)
        stl_file.write(struct.pack("<I", len(triangles)))
        for triangle in triangles:
            nx, ny, nz = normal(triangle)
            values = [nx, ny, nz]
            for vertex in triangle:
                values.extend(vertex)
            stl_file.write(struct.pack("<12fH", *values, 0))


def is_inside_rect(x: float, y: float, rect: tuple[float, float, float, float]) -> bool:
    rx, ry, rw, rd = rect
    return rx <= x <= rx + rw and ry <= y <= ry + rd


def is_inside_circle(x: float, y: float, circle: tuple[float, float, float]) -> bool:
    cx, cy, diameter = circle
    radius = diameter / 2
    return (x - cx) * (x - cx) + (y - cy) * (y - cy) <= radius * radius


def generate_cut_plate(
    width: float,
    depth: float,
    thickness: float,
    rects: list[tuple[float, float, float, float]],
    circles: list[tuple[float, float, float]],
    resolution: float = 1.0,
) -> list[Triangle]:
    xs = {0.0, width}
    ys = {0.0, depth}
    x = 0.0
    while x < width:
        xs.add(round(x, 4))
        x += resolution
    y = 0.0
    while y < depth:
        ys.add(round(y, 4))
        y += resolution

    for rx, ry, rw, rd in rects:
        xs.update([rx, rx + rw])
        ys.update([ry, ry + rd])
    for cx, cy, diameter in circles:
        radius = diameter / 2
        xs.update([cx - radius, cx, cx + radius])
        ys.update([cy - radius, cy, cy + radius])

    x_values = sorted(value for value in xs if 0 <= value <= width)
    y_values = sorted(value for value in ys if 0 <= value <= depth)
    filled: dict[tuple[int, int], tuple[float, float, float, float]] = {}

    for xi in range(len(x_values) - 1):
        for yi in range(len(y_values) - 1):
            x0, x1 = x_values[xi], x_values[xi + 1]
            y0, y1 = y_values[yi], y_values[yi + 1]
            if x1 <= x0 or y1 <= y0:
                continue
            cx = (x0 + x1) / 2
            cy = (y0 + y1) / 2
            cut = any(is_inside_rect(cx, cy, rect) for rect in rects) or any(is_inside_circle(cx, cy, circle) for circle in circles)
            if not cut:
                filled[(xi, yi)] = (x0, y0, x1, y1)

    triangles: list[Triangle] = []
    for (xi, yi), (x0, y0, x1, y1) in filled.items():
        p00b = (x0, y0, 0.0)
        p10b = (x1, y0, 0.0)
        p11b = (x1, y1, 0.0)
        p01b = (x0, y1, 0.0)
        p00t = (x0, y0, thickness)
        p10t = (x1, y0, thickness)
        p11t = (x1, y1, thickness)
        p01t = (x0, y1, thickness)
        add_quad(triangles, p00b, p10b, p11b, p01b)
        add_quad(triangles, p00t, p01t, p11t, p10t)
        if (xi, yi - 1) not in filled:
            add_quad(triangles, p00b, p00t, p10t, p10b)
        if (xi + 1, yi) not in filled:
            add_quad(triangles, p10b, p10t, p11t, p11b)
        if (xi, yi + 1) not in filled:
            add_quad(triangles, p11b, p11t, p01t, p01b)
        if (xi - 1, yi) not in filled:
            add_quad(triangles, p01b, p01t, p00t, p00b)

    return triangles


def translate_triangles(source: list[Triangle], dx: float, dy: float, dz: float) -> list[Triangle]:
    return [
        tuple((x + dx, y + dy, z + dz) for x, y, z in triangle)  # type: ignore[misc]
        for triangle in source
    ]


def generate_cut_wall_y(
    width: float,
    height: float,
    thickness: float,
    rects: list[tuple[float, float, float, float]],
    circles: list[tuple[float, float, float]],
    resolution: float = 0.5,
) -> list[Triangle]:
    xs = {0.0, width}
    zs = {0.0, height}
    x = 0.0
    while x < width:
        xs.add(round(x, 4))
        x += resolution
    z = 0.0
    while z < height:
        zs.add(round(z, 4))
        z += resolution

    for rx, rz, rw, rh in rects:
        xs.update([rx, rx + rw])
        zs.update([rz, rz + rh])
    for cx, cz, diameter in circles:
        radius = diameter / 2
        xs.update([cx - radius, cx, cx + radius])
        zs.update([cz - radius, cz, cz + radius])

    x_values = sorted(value for value in xs if 0 <= value <= width)
    z_values = sorted(value for value in zs if 0 <= value <= height)
    filled: dict[tuple[int, int], tuple[float, float, float, float]] = {}

    for xi in range(len(x_values) - 1):
        for zi in range(len(z_values) - 1):
            x0, x1 = x_values[xi], x_values[xi + 1]
            z0, z1 = z_values[zi], z_values[zi + 1]
            if x1 <= x0 or z1 <= z0:
                continue
            cx = (x0 + x1) / 2
            cz = (z0 + z1) / 2
            cut = any(is_inside_rect(cx, cz, rect) for rect in rects) or any(is_inside_circle(cx, cz, circle) for circle in circles)
            if not cut:
                filled[(xi, zi)] = (x0, z0, x1, z1)

    triangles: list[Triangle] = []
    for (xi, zi), (x0, z0, x1, z1) in filled.items():
        p00f = (x0, 0.0, z0)
        p10f = (x1, 0.0, z0)
        p11f = (x1, 0.0, z1)
        p01f = (x0, 0.0, z1)
        p00b = (x0, thickness, z0)
        p10b = (x1, thickness, z0)
        p11b = (x1, thickness, z1)
        p01b = (x0, thickness, z1)
        add_quad(triangles, p00f, p10f, p11f, p01f)
        add_quad(triangles, p00b, p01b, p11b, p10b)
        if (xi, zi - 1) not in filled:
            add_quad(triangles, p00f, p00b, p10b, p10f)
        if (xi + 1, zi) not in filled:
            add_quad(triangles, p10f, p10b, p11b, p11f)
        if (xi, zi + 1) not in filled:
            add_quad(triangles, p11f, p11b, p01b, p01f)
        if (xi - 1, zi) not in filled:
            add_quad(triangles, p01f, p01b, p00b, p00f)

    return triangles


def add_back_wall_segment(triangles: list[Triangle], x0: float, x1: float, height: float) -> None:
    radius = DC_JACK_HOLE_D / 2
    jack_in_segment = x0 + radius <= DC_JACK_CENTER_X <= x1 - radius and radius <= DC_JACK_CENTER_Z <= height - radius
    if not jack_in_segment:
        add_box(triangles, x0, PANEL_D - WALL, 0, x1, PANEL_D, height)
        return

    wall = generate_cut_wall_y(
        x1 - x0,
        height,
        WALL,
        rects=[],
        circles=[(DC_JACK_CENTER_X - x0, DC_JACK_CENTER_Z, DC_JACK_HOLE_D)],
        resolution=0.5,
    )
    triangles.extend(translate_triangles(wall, x0, PANEL_D - WALL, 0))


def add_back_wall_with_notches(triangles: list[Triangle]) -> None:
    slots = [(190.0, 44.0, 18.0), (236.0, 34.0, 16.0)]
    cursor = 0.0
    for slot_x, slot_w, slot_depth in slots:
        if cursor < slot_x:
            add_back_wall_segment(triangles, cursor, slot_x, BASE_H)
        add_back_wall_segment(triangles, slot_x, slot_x + slot_w, BASE_H - slot_depth)
        cursor = slot_x + slot_w
    if cursor < PANEL_W:
        add_back_wall_segment(triangles, cursor, PANEL_W, BASE_H)


def add_right_wall_with_load_exit(triangles: list[Triangle]) -> None:
    slot_y = 112.0
    slot_d = 40.0
    slot_depth = 22.0
    add_box(triangles, PANEL_W - WALL, 0, 0, PANEL_W, slot_y, BASE_H)
    add_box(triangles, PANEL_W - WALL, slot_y, 0, PANEL_W, slot_y + slot_d, BASE_H - slot_depth)
    add_box(triangles, PANEL_W - WALL, slot_y + slot_d, 0, PANEL_W, PANEL_D, BASE_H)


def base_tray() -> list[Triangle]:
    triangles: list[Triangle] = []
    add_box(triangles, 0, 0, 0, PANEL_W, PANEL_D, FLOOR_T)
    add_box(triangles, 0, 0, 0, PANEL_W, WALL, BASE_H)
    add_back_wall_with_notches(triangles)
    add_box(triangles, 0, 0, 0, WALL, PANEL_D, BASE_H)
    add_right_wall_with_load_exit(triangles)

    for x in (12.0, PANEL_W - 12.0):
        for y in (12.0, PANEL_D - 12.0):
            add_cylinder_shell(triangles, x, y, FLOOR_T, FLOOR_T + 14.0, 4.8, 1.6)

    for dx, dy in PCB_HOLES:
        add_cylinder_shell(triangles, PCB_X + dx, PCB_Y + dy, FLOOR_T, FLOOR_T + PCB_STANDOFF_H, 4.8, 1.7)

    # Raised rails mark internal keepout zones without depending on unknown module hole spacing.
    add_box(triangles, 166, 38, FLOOR_T, 244, 41, FLOOR_T + 2.0)   # XL4015 rear rail
    add_box(triangles, 166, 84, FLOOR_T, 244, 87, FLOOR_T + 2.0)   # XL4015 front rail
    add_box(triangles, 166, 100, FLOOR_T, 232, 103, FLOOR_T + 2.0)  # relay rear rail
    add_box(triangles, 166, 153, FLOOR_T, 232, 156, FLOOR_T + 2.0)  # relay front rail
    add_box(triangles, 248, 112, FLOOR_T, 252, 152, FLOOR_T + 8.0)  # load-wire strain-relief post
    add_box(triangles, 258, 112, FLOOR_T, 262, 152, FLOOR_T + 8.0)  # load-wire strain-relief post
    return triangles


def top_panel() -> list[Triangle]:
    rects = [
        (25.0, 27.0, 79.0, 28.0),     # LCD display window, kept clear of mount holes
        (170.0, 96.0, 24.0, 8.0),     # keypad cable/header slot
        (238.0, 24.0, 30.0, 24.0),    # BME280 room-air recess
        (239.0, 58.0, 24.0, 4.0),     # BME280 vent slot
        (239.0, 66.0, 24.0, 4.0),     # BME280 vent slot
        (236.0, 143.0, 15.0, 10.0),   # KCD11 rocker switch cutout
        (242.0, 178.0, 34.0, 12.0),   # service cable top notch
    ]
    circles: list[tuple[float, float, float]] = []

    for x in (19.5, 109.3):
        for y in (20.5, 62.0):
            circles.append((x, y, 3.4))

    for x in (28, 44, 60, 76, 92, 112, 132):
        circles.append((float(x), 116.0, 7.6))

    for x in (30, 62, 94):
        circles.append((float(x), 152.0, 12.4))
    circles.append((132.0, 152.0, 7.4))

    for dx in (-6, 0, 6):
        for dy in (-6, 0, 6):
            circles.append((186.0 + dx, 152.0 + dy, 2.6))

    for x in (232.0, 272.0):
        circles.append((x, 36.0, 3.2))

    for x in (12.0, PANEL_W - 12.0):
        for y in (12.0, PANEL_D - 12.0):
            circles.append((x, y, 3.4))

    return generate_cut_plate(PANEL_W, PANEL_D, PANEL_T, rects, circles, resolution=1.0)


def bme280_vent_cage() -> list[Triangle]:
    triangles: list[Triangle] = []
    base = generate_cut_plate(56.0, 44.0, 2.0, [(7.0, 5.0, 42.0, 34.0)], [(8.0, 22.0, 3.2), (48.0, 22.0, 3.2)], resolution=0.5)
    triangles.extend(base)
    add_box(triangles, 7, 5, 2, 10, 39, 14)
    add_box(triangles, 46, 5, 2, 49, 39, 14)
    add_box(triangles, 7, 5, 2, 49, 8, 14)
    add_box(triangles, 7, 36, 2, 49, 39, 14)
    for y in (10.0, 18.0, 26.0, 34.0):
        add_box(triangles, 11.0, y, 13.0, 45.0, y + 2.0, 15.0)
    return triangles


def hcsr04_faceplate() -> list[Triangle]:
    circles = [
        (22.0, 17.5, 18.2),
        (48.0, 17.5, 18.2),
        (8.0, 8.0, 3.2),
        (62.0, 8.0, 3.2),
        (8.0, 27.0, 3.2),
        (62.0, 27.0, 3.2),
    ]
    return generate_cut_plate(70.0, 35.0, 3.0, [], circles, resolution=0.5)


def hcsr04_mast_base() -> list[Triangle]:
    rects = [(35.5, 30.5, 19.0, 19.0)]
    circles = [
        (10.0, 10.0, 3.4),
        (80.0, 10.0, 3.4),
        (10.0, 70.0, 3.4),
        (80.0, 70.0, 3.4),
    ]
    return generate_cut_plate(90.0, 80.0, 8.0, rects, circles, resolution=0.5)


def hcsr04_mast_upright() -> list[Triangle]:
    triangles: list[Triangle] = []
    add_box(triangles, 0, 0, 0, 18.0, 18.0, 132.0)
    return triangles


def hcsr04_mast_arm() -> list[Triangle]:
    rects = [(8.5, 8.0, 19.0, 19.0)]
    circles = [
        (54.0, 8.0, 3.4),
        (54.0, 27.0, 3.4),
        (108.0, 8.0, 3.4),
        (108.0, 27.0, 3.4),
    ]
    return generate_cut_plate(116.0, 35.0, 8.0, rects, circles, resolution=0.5)


def main() -> int:
    out_dir = Path(__file__).resolve().parent / "stl"
    files = {
        "bahayshield-base-tray.stl": base_tray(),
        "bahayshield-top-panel.stl": top_panel(),
        "bme280-vent-cage.stl": bme280_vent_cage(),
        "hcsr04-faceplate.stl": hcsr04_faceplate(),
        "hcsr04-mast-base.stl": hcsr04_mast_base(),
        "hcsr04-mast-upright-132mm.stl": hcsr04_mast_upright(),
        "hcsr04-mast-arm.stl": hcsr04_mast_arm(),
    }
    for filename, triangles in files.items():
        write_stl(out_dir / filename, filename.removesuffix(".stl"), triangles)
    print(f"Generated {len(files)} STL files in {out_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
