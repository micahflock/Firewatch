// Firewatch mounting hardware — parametric, OpenSCAD 2021+.
//
//   part = "base"  VESA plate + enclosure for a QuinLED Dig-Uno v3 (with QuinLED-ESP32)
//   part = "lid"   vented lid
//   part = "clip"  45-degree LED strip clip (stick to the TV back with VHB tape)
//   part = "all"   assembly preview (not for printing)
//
// Export:  openscad -D 'part="base"' -o base.stl firewatch_mount.scad
// Units: mm. Print in PETG (or PLA — the TV back stays well under PLA's softening point).
//
// Board orientation (matches QuinLED's dimension drawing, viewed from above):
//   -X edge: power input terminal + the ESP32's USB-C port
//   +X edge: LED output terminal (5V / LED1 / LED2 / GND)
//   -Y edge: Q1/Q2 header (optional IR LED lead)

part = "all";

/* [VESA] */
// Slots run diagonally from the 75x75 to the 100x100 hole positions, so one plate fits
// either pattern (the TCL 32S335 is 100x100, M4).
vesa_min = 75;
vesa_max = 100;
screw_d = 4.5;        // M4 clearance; use 6.6 for M6 TVs
plate_t = 2;          // solid floor; 2 mm is plenty for PETG
arm_w = 14;           // width of the arms around the VESA slots

/* [Dig-Uno v3] */
// From QuinLED's published dimension drawing.
pcb_x = 48.52;
pcb_y = 39.37;
hole_dx = 43.27;      // M2 hole centers
hole_dy = 34.02;
stack_h = 26;         // PCB underside to top of the tallest part (ESP32/capacitor), measured
standoff_h = 5;
standoff_d = 5.5;
m2_pilot = 1.6;       // M2 self-tapping into plastic (use 3.2 for M2 heat-set inserts)
margin_x = 9;         // room for the terminal wires to bend, both ends
margin_y = 5;
// M2 x 8 through the 1.6 mm PCB reaches 6.4 mm down; keep a 0.4 mm skin under the hole.
pilot_depth = min(7, plate_t + standoff_h - 0.4);

/* [Box] */
// Thin vertical walls: 1.67 mm suits a 0.4 mm nozzle at the usual layer heights.
thin_wall = 1.67;
wall = thin_wall;
inner_h = standoff_h + stack_h + 2;
power_window_w = 36;  // -X wall: power wires + USB-C access
led_notch_w = 18;     // +X wall: LED output wires
window_floor = 8;     // openings run from here to the top of the wall
ir_notch_w = 10;      // -Y wall: optional IR lead from the Q1/Q2 header
ir_notch_x = -3;
ir_notch_depth = 14;

/* [Lid] */
lid_t = 2;
lip_h = 2;
lip_t = thin_wall;
fit = 0.3;
lid_screw_clear = 2.4; // M2
boss_d = 7;
boss_x = 20;

/* [Strip clip] */
strip_w = 10;          // PCB width of the LED strip (10 or 12 mm are common)
clip_angle = 45;       // 0 = face straight back, 90 = face straight out
clip_len = 20;
clip_wall = thin_wall;   // the clip prints profile face down, so every thickness
                         // in its profile is a vertical wall
clip_floor = thin_wall;
pcb_slot = 0.7;        // PCB + adhesive thickness under the retaining lips
lip_over = 1.0;        // how far the lips overlap the PCB edge (clear of the LEDs)

$fn = 48;
eps = 0.01;

in_x = pcb_x + 2 * margin_x;
in_y = pcb_y + 2 * margin_y;
out_x = in_x + 2 * wall;
out_y = in_y + 2 * wall;
boss_y = out_y / 2 + boss_d / 2 - wall;
anchor_x = out_x / 2 + 8;

module rounded_square(size, r) {
  offset(r) square([size[0] - 2 * r, size[1] - 2 * r], center = true);
}

module vesa_slots() {
  for (sx = [-1, 1], sy = [-1, 1])
    hull() {
      translate([sx * vesa_min / 2, sy * vesa_min / 2, -eps]) cylinder(d = screw_d, h = plate_t + 2 * eps);
      translate([sx * vesa_max / 2, sy * vesa_max / 2, -eps]) cylinder(d = screw_d, h = plate_t + 2 * eps);
    }
}

module zip_anchor() {
  // Bridge a cable tie passes under; the cable runs along X over it.
  tie_w = 4;
  aw = tie_w + 2 * thin_wall;
  difference() {
    translate([-aw / 2, -5, 0]) cube([aw, 10, 5]);
    translate([-tie_w / 2, -5 - eps, 1.2]) cube([tie_w, 10 + 2 * eps, 2.2]);
  }
}

module plate_2d() {
  // Pad under the box and cable anchors, plus four arms out to the VESA slots.
  rounded_square([2 * anchor_x + 12, out_y + 8], 4);
  for (sx = [-1, 1], sy = [-1, 1])
    hull() {
      translate([sx * 12, sy * 12]) circle(d = 18);
      translate([sx * vesa_min / 2, sy * vesa_min / 2]) circle(d = arm_w);
      translate([sx * vesa_max / 2, sy * vesa_max / 2]) circle(d = arm_w);
    }
}

module base() {
  difference() {
    union() {
      linear_extrude(plate_t) plate_2d();
      translate([0, 0, plate_t]) linear_extrude(inner_h)
        difference() {
          rounded_square([out_x, out_y], 2);
          square([in_x, in_y], center = true);
        }
      for (sx = [-1, 1], sy = [-1, 1])
        translate([sx * boss_x, sy * boss_y, 0]) cylinder(d = boss_d, h = plate_t + inner_h);
      for (sx = [-1, 1], sy = [-1, 1])
        translate([sx * hole_dx / 2, sy * hole_dy / 2, plate_t - eps]) cylinder(d = standoff_d, h = standoff_h + eps);
      // two anchors on the power side (barrel jack pigtail), one on the LED side (strip wiring)
      for (y = [-9, 9]) translate([-anchor_x, y, plate_t - eps]) zip_anchor();
      translate([anchor_x, 0, plate_t - eps]) zip_anchor();
    }
    vesa_slots();
    for (sx = [-1, 1], sy = [-1, 1]) {
      translate([sx * hole_dx / 2, sy * hole_dy / 2, plate_t + standoff_h - pilot_depth]) cylinder(d = m2_pilot, h = pilot_depth + eps);
      translate([sx * boss_x, sy * boss_y, plate_t + inner_h - 8]) cylinder(d = m2_pilot, h = 8 + eps);
    }
    translate([-out_x / 2 - 1, -power_window_w / 2, plate_t + window_floor]) cube([wall + 2, power_window_w, inner_h]);
    translate([out_x / 2 - wall - 1, -led_notch_w / 2, plate_t + window_floor]) cube([wall + 2, led_notch_w, inner_h]);
    translate([ir_notch_x - ir_notch_w / 2, -out_y / 2 - 1, plate_t + inner_h - ir_notch_depth])
      cube([ir_notch_w, wall + 2, ir_notch_depth + eps]);
  }
}

module lid() {
  // Modeled upside down relative to the base: printed top face down on the bed.
  difference() {
    union() {
      linear_extrude(lid_t) rounded_square([out_x, out_y], 2);
      for (sx = [-1, 1], sy = [-1, 1])
        translate([sx * boss_x, sy * boss_y, 0]) cylinder(d = boss_d, h = lid_t);
      translate([0, 0, lid_t - eps]) linear_extrude(lip_h)
        difference() {
          square([in_x - fit, in_y - fit], center = true);
          square([in_x - fit - 2 * lip_t, in_y - fit - 2 * lip_t], center = true);
        }
    }
    for (sx = [-1, 1], sy = [-1, 1])
      translate([sx * boss_x, sy * boss_y, -eps]) cylinder(d = lid_screw_clear, h = lid_t + 2 * eps);
    for (i = [-4 : 4])
      translate([i * 6 - 1, -15, -eps]) cube([2, 30, lid_t + 2 * eps]);
  }
}

module board_ghost() {
  // Dig-Uno envelope, for the assembly preview only.
  translate([0, 0, plate_t + standoff_h]) {
    color("royalblue") translate([-pcb_x / 2, -pcb_y / 2, 0]) cube([pcb_x, pcb_y, 1.6]);
    color("silver", 0.5) translate([-pcb_x / 2 + 6, -pcb_y / 2 + 3, 1.6]) cube([pcb_x - 8, pcb_y - 6, stack_h - 1.6]);
  }
}

module clip_profile() {
  w = strip_w + 2 * clip_wall + 0.4;
  h = clip_floor + pcb_slot + thin_wall;
  a = clip_angle;
  channel = [
    [0, 0], [w, 0], [w, h],
    [w - clip_wall - lip_over, h], [w - clip_wall - lip_over, clip_floor + pcb_slot],
    [w - clip_wall, clip_floor + pcb_slot], [w - clip_wall, clip_floor],
    [clip_wall, clip_floor], [clip_wall, clip_floor + pcb_slot],
    [clip_wall + lip_over, clip_floor + pcb_slot], [clip_wall + lip_over, h],
    [0, h]];
  // Profile plane: x across the TV surface (-x points toward the TV edge), y away from
  // the TV. The mounting face is y = 0; the channel opens outward and backward.
  base_t = thin_wall;
  translate([0, base_t]) {
    rotate(a) polygon(channel);
    if (a > 0) polygon([[0, 0], [w * cos(a), w * sin(a)], [w * cos(a), 0]]);
  }
  translate([-h * sin(a) - 1, 0]) square([h * sin(a) + w * cos(a) + 2, base_t]);
}

module clip() {
  linear_extrude(clip_len) clip_profile();
}

if (part == "base") base();
else if (part == "lid") lid();
else if (part == "clip") clip();
else {
  base();
  board_ghost();
  translate([0, 0, plate_t + inner_h + lid_t + 25]) mirror([0, 0, 1]) color("orange", 0.6) lid();
  translate([85, 0, 0]) clip();
}
