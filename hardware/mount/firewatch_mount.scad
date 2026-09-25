// Firewatch mounting hardware — parametric, OpenSCAD 2021+.
//
//   part = "base"  VESA plate + electronics box (holds a 50x70 mm perfboard)
//   part = "lid"   box lid with vents; presses the board down at its corners
//   part = "clip"  45-degree LED strip clip (stick to the TV back with VHB tape)
//   part = "all"   assembly preview (not for printing)
//
// Export:  openscad -D 'part="base"' -o base.stl firewatch_mount.scad
// Units: mm. Print in PETG (or PLA — the TV back stays well under PLA's softening point).

part = "all";

/* [VESA] */
// Slots run diagonally from the 75x75 to the 100x100 hole positions, so one plate fits
// either pattern (the TCL 32S335 is 100x100).
vesa_min = 75;
vesa_max = 100;
screw_d = 4.5;        // M4 clearance; use 6.6 for M6 TVs
plate_t = 4;
arm_w = 14;           // width of the arms around the VESA slots

/* [Board] */
board_w = 50;         // standard 50x70 perfboard
board_l = 70;
board_t = 1.6;
board_gap = 0.5;      // total clearance per axis
standoff_h = 4;       // room for solder joints under the board
corner_keepout = 5;   // keep this much of each board corner free of parts

/* [Box] */
wall = 2.4;
inner_h = 26;         // floor to lid underside; fits an ESP32 DevKit on female headers
usb_notch_w = 13;
usb_notch_bottom = 13; // notch floor above the plate; ESP32 USB sits ~17 mm up
cable_notch_w = 8;
cable_notch_depth = 10;
cable_notch_x = 12;   // +/- offset of the two cable notches (power in, strip out)

/* [Lid] */
lid_t = 2.4;
lip_h = 2;
lip_t = 1.2;
fit = 0.3;
lid_screw_pilot = 2.7; // M3 self-tapping into plastic (use 4.0 for M3 heat-set inserts)
lid_screw_clear = 3.4;
boss_d = 8;
boss_y = 20;

/* [Strip clip] */
strip_w = 10;          // PCB width of the LED strip (10 or 12 mm are common)
clip_angle = 45;       // 0 = face straight back, 90 = face straight out
clip_len = 20;
clip_wall = 1.4;
clip_floor = 1.6;
pcb_slot = 0.7;        // PCB + adhesive thickness under the retaining lips
lip_over = 1.0;        // how far the lips overlap the PCB edge (clear of the LEDs)

$fn = 48;
eps = 0.01;

in_w = board_w + board_gap;
in_l = board_l + board_gap;
out_w = in_w + 2 * wall;
out_l = in_l + 2 * wall;

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
  // Bridge a cable tie passes under; the cable runs along Y beside it.
  difference() {
    translate([-5, -3, 0]) cube([10, 6, 5]);
    translate([-5 - eps, -2, 1.2]) cube([10 + 2 * eps, 4, 2.2]);
  }
}

module plate_2d() {
  // Pad under the box and cable anchors, plus four arms out to the VESA slots.
  translate([0, -6]) rounded_square([out_w + 8, out_l + 22], 4);
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
      // box walls
      translate([0, 0, plate_t]) linear_extrude(inner_h)
        difference() {
          rounded_square([out_w, out_l], 2);
          square([in_w, in_l], center = true);
        }
      // lid screw bosses on the long sides, clear of the VESA slots
      for (sx = [-1, 1], sy = [-1, 1])
        translate([sx * (out_w / 2 + boss_d / 2 - wall), sy * boss_y, 0])
          cylinder(d = boss_d, h = plate_t + inner_h);
      // board standoffs in the corners
      for (sx = [-1, 1], sy = [-1, 1])
        translate([sx * (in_w / 2 - corner_keepout / 2), sy * (in_l / 2 - corner_keepout / 2), plate_t + standoff_h / 2])
          cube([corner_keepout, corner_keepout, standoff_h], center = true);
      // cable tie anchors outside the cable end
      for (sx = [-1, 1])
        translate([sx * cable_notch_x, -(out_l / 2 + 8), plate_t - eps]) zip_anchor();
    }
    vesa_slots();
    // pilot holes for lid screws
    for (sx = [-1, 1], sy = [-1, 1])
      translate([sx * (out_w / 2 + boss_d / 2 - wall), sy * boss_y, plate_t + inner_h - 12])
        cylinder(d = lid_screw_pilot, h = 12 + eps);
    // USB access (ESP32 end, +Y)
    translate([-usb_notch_w / 2, out_l / 2 - wall - 1, plate_t + usb_notch_bottom])
      cube([usb_notch_w, wall + 2, inner_h]);
    // cable notches (terminal end, -Y)
    for (sx = [-1, 1])
      translate([sx * cable_notch_x - cable_notch_w / 2, -out_l / 2 - 1, plate_t + inner_h - cable_notch_depth])
        cube([cable_notch_w, wall + 2, cable_notch_depth + eps]);
  }
}

module lid() {
  // Modeled upside down relative to the base: printed top face down on the bed.
  post_h = inner_h - standoff_h - board_t - 0.1;
  difference() {
    union() {
      linear_extrude(lid_t) rounded_square([out_w, out_l], 2);
      for (sx = [-1, 1], sy = [-1, 1])
        translate([sx * (out_w / 2 + boss_d / 2 - wall), sy * boss_y, 0]) cylinder(d = boss_d, h = lid_t);
      // locating lip just inside the walls
      translate([0, 0, lid_t - eps]) linear_extrude(lip_h)
        difference() {
          square([in_w - fit, in_l - fit], center = true);
          square([in_w - fit - 2 * lip_t, in_l - fit - 2 * lip_t], center = true);
        }
      // corner posts that clamp the board onto the standoffs
      for (sx = [-1, 1], sy = [-1, 1])
        translate([sx * (in_w / 2 - corner_keepout / 2 - fit), sy * (in_l / 2 - corner_keepout / 2 - fit), lid_t - eps])
          linear_extrude(post_h) square([corner_keepout - 1, corner_keepout - 1], center = true);
    }
    for (sx = [-1, 1], sy = [-1, 1])
      translate([sx * (out_w / 2 + boss_d / 2 - wall), sy * boss_y, -eps]) cylinder(d = lid_screw_clear, h = lid_t + 2 * eps);
    // vent slots
    for (i = [-3 : 3])
      translate([i * 5.5 - 1, -18, -eps]) cube([2, 36, lid_t + 2 * eps]);
  }
}

module clip_profile() {
  w = strip_w + 2 * clip_wall + 0.4;
  h = clip_floor + pcb_slot + 1.0;
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
  base_t = 1.6;
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
  translate([0, 0, plate_t + inner_h + lid_t]) mirror([0, 0, 1]) color("orange", 0.6) lid();
  translate([85, 0, 0]) clip();
}
