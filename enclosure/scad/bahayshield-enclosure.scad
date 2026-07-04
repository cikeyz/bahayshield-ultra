$fn = 64;

panel_w = 280;
panel_d = 200;
panel_t = 3.2;
base_h = 68;
wall = 3;
floor_t = 3;

pcb_x = 18;
pcb_y = 42;
pcb_size = 125;
pcb_standoff_h = 10;
pcb_holes = [[6.0, 6.0], [120.3, 5.3], [6.4, 118.6], [119.2, 119.1]];
dc_jack_x = 253;
dc_jack_z = 34;
dc_jack_d = 12.4;

module hole(d, h = 20) {
    cylinder(d = d, h = h, center = true);
}

module back_wall_segment(x0, w, h) {
    difference() {
        translate([x0, panel_d - wall, 0]) cube([w, wall, h]);
        if (dc_jack_x >= x0 + dc_jack_d / 2 && dc_jack_x <= x0 + w - dc_jack_d / 2 && dc_jack_z >= dc_jack_d / 2 && dc_jack_z <= h - dc_jack_d / 2) {
            translate([dc_jack_x, panel_d - wall / 2, dc_jack_z]) rotate([90, 0, 0]) hole(dc_jack_d, wall + 2);
        }
    }
}

module base_tray() {
    cube([panel_w, panel_d, floor_t]);
    cube([panel_w, wall, base_h]);
    cube([wall, panel_d, base_h]);

    // Back wall with service notches and 12.4 mm DC barrel jack opening.
    back_wall_segment(0, 190, base_h);
    back_wall_segment(190, 44, base_h - 18);
    back_wall_segment(234, 2, base_h);
    back_wall_segment(236, 34, base_h - 16);
    back_wall_segment(270, 10, base_h);

    // Right wall with low-voltage load-wire exit.
    translate([panel_w - wall, 0, 0]) cube([wall, 112, base_h]);
    translate([panel_w - wall, 112, 0]) cube([wall, 40, base_h - 22]);
    translate([panel_w - wall, 152, 0]) cube([wall, panel_d - 152, base_h]);

    for (x = [12, panel_w - 12]) {
        for (y = [12, panel_d - 12]) {
            translate([x, y, floor_t]) difference() {
                cylinder(d = 9.6, h = 14);
                translate([0, 0, -1]) cylinder(d = 3.2, h = 16);
            }
        }
    }

    for (p = pcb_holes) {
        translate([pcb_x + p[0], pcb_y + p[1], floor_t]) difference() {
            cylinder(d = 9.6, h = pcb_standoff_h);
            translate([0, 0, -1]) cylinder(d = 3.4, h = pcb_standoff_h + 2);
        }
    }

    // Internal rails mark module zones and give cable-tie anchor points.
    translate([166, 38, floor_t]) cube([78, 3, 2]);
    translate([166, 84, floor_t]) cube([78, 3, 2]);
    translate([166, 100, floor_t]) cube([66, 3, 2]);
    translate([166, 153, floor_t]) cube([66, 3, 2]);
    translate([248, 112, floor_t]) cube([4, 40, 8]);
    translate([258, 112, floor_t]) cube([4, 40, 8]);
}

module top_panel() {
    difference() {
        cube([panel_w, panel_d, panel_t]);

        // LCD, keypad, BME280 vent/recess, rocker switch, and service notch.
        translate([25, 27, -1]) cube([79, 28, panel_t + 2]);
        translate([170, 96, -1]) cube([24, 8, panel_t + 2]);
        translate([238, 24, -1]) cube([30, 24, panel_t + 2]);
        translate([239, 58, -1]) cube([24, 4, panel_t + 2]);
        translate([239, 66, -1]) cube([24, 4, panel_t + 2]);
        translate([236, 143, -1]) cube([15, 10, panel_t + 2]);
        translate([242, 178, -1]) cube([34, 12, panel_t + 2]);

        for (x = [19.5, 109.3]) {
            for (y = [20.5, 62]) {
                translate([x, y, panel_t / 2]) hole(3.4, panel_t + 2);
            }
        }

        for (x = [28, 44, 60, 76, 92, 112, 132]) {
            translate([x, 116, panel_t / 2]) hole(7.6, panel_t + 2);
        }

        for (x = [30, 62, 94]) {
            translate([x, 152, panel_t / 2]) hole(12.4, panel_t + 2);
        }

        translate([132, 152, panel_t / 2]) hole(7.4, panel_t + 2);

        for (dx = [-6, 0, 6]) {
            for (dy = [-6, 0, 6]) {
                translate([186 + dx, 152 + dy, panel_t / 2]) hole(2.6, panel_t + 2);
            }
        }

        for (x = [232, 272]) {
            translate([x, 36, panel_t / 2]) hole(3.2, panel_t + 2);
        }

        for (x = [12, panel_w - 12]) {
            for (y = [12, panel_d - 12]) {
                translate([x, y, panel_t / 2]) hole(3.4, panel_t + 2);
            }
        }
    }
}

module bme280_vent_cage() {
    difference() {
        cube([56, 44, 2]);
        translate([7, 5, -1]) cube([42, 34, 4]);
        translate([8, 22, 1]) hole(3.2, 4);
        translate([48, 22, 1]) hole(3.2, 4);
    }
    translate([7, 5, 2]) cube([3, 34, 12]);
    translate([46, 5, 2]) cube([3, 34, 12]);
    translate([7, 5, 2]) cube([42, 3, 12]);
    translate([7, 36, 2]) cube([42, 3, 12]);
    for (y = [10, 18, 26, 34]) {
        translate([11, y, 13]) cube([34, 2, 2]);
    }
}

module hcsr04_faceplate() {
    difference() {
        cube([70, 35, 3]);
        translate([22, 17.5, 1.5]) hole(18.2, 5);
        translate([48, 17.5, 1.5]) hole(18.2, 5);
        translate([8, 8, 1.5]) hole(3.2, 5);
        translate([62, 8, 1.5]) hole(3.2, 5);
        translate([8, 27, 1.5]) hole(3.2, 5);
        translate([62, 27, 1.5]) hole(3.2, 5);
    }
}

module hcsr04_mast_base() {
    difference() {
        cube([90, 80, 8]);
        translate([35.5, 30.5, -1]) cube([19, 19, 10]);
        for (x = [10, 80]) {
            for (y = [10, 70]) {
                translate([x, y, 4]) hole(3.4, 10);
            }
        }
    }
}

module hcsr04_mast_upright() {
    cube([18, 18, 132]);
}

module hcsr04_mast_arm() {
    difference() {
        cube([116, 35, 8]);
        translate([8.5, 8, -1]) cube([19, 19, 10]);
        for (x = [54, 108]) {
            for (y = [8, 27]) {
                translate([x, y, 4]) hole(3.4, 10);
            }
        }
    }
}

base_tray();
translate([0, panel_d + 20, 0]) top_panel();
translate([0, panel_d + 240, 0]) bme280_vent_cage();
translate([80, panel_d + 240, 0]) hcsr04_faceplate();
translate([0, panel_d + 310, 0]) hcsr04_mast_base();
translate([110, panel_d + 310, 0]) hcsr04_mast_upright();
translate([150, panel_d + 310, 0]) hcsr04_mast_arm();
