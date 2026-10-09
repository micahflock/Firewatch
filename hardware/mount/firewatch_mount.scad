// Firewatch mounting hardware — parametric, OpenSCAD 2021+.
//
//   part = "base"           VESA plate + enclosure for a QuinLED Dig-Uno v3 (with QuinLED-ESP32)
//   part = "lid"            vented lid
//   part = "clip_base"      45-degree LED strip seat (stick to the TV back with VHB tape)
//   part = "clip_cap"       snap-on cap that holds the strip on the seat
//   part = "clip_assembly"  strip clip preview with the cap in place (not for printing)
//   part = "all"            assembly preview (not for printing)
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
// Vertical wall thicknesses that slice cleanly with a 0.4 mm nozzle: strong_wall for
// walls that take load, light_wall for walls that only locate or guide.
strong_wall = 1.67;
light_wall = 0.86;
wall = strong_wall;
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
lip_t = light_wall;
fit = 0.3;
lid_screw_clear = 2.4; // M2
boss_d = 7;
boss_x = 20;

/* [Strip clip] */
// Two parts: a base with a flat seat that sticks to the TV, and a narrow cap that snaps
// over the seat between two LEDs and holds the strip's edges down. The strip just lies on
// the seat, so it never has to bend across its width.
strip_w = 10;          // PCB width of the LED strip (10 or 12 mm are common)
clip_angle = 45;       // 0 = face straight back, 90 = face straight out
clip_len = 20;         // base length along the strip
cap_len = 5;           // cap length along the strip; it fits the 11.7 mm gap between
                       // LEDs on a 60 LED/m strip
strip_fit = 0.4;       // seat width over the strip width
pcb_slot = 0.5;        // PCB + adhesive thickness under the cap's lips; at or a little
                       // under the real thickness so the lips clamp the strip
lip_over = 1.0;        // how far the lips overlap the PCB edge (clear of the LEDs)
comp_clear = 1.0;      // headroom over the PCB between the lips (the capacitor beside
                       // each LED)
seat_t = strong_wall;  // the seat plate; its underside is the ledge the cap hooks under
snap_over = 0.5;       // how far the hooks reach under the seat
snap_fit = 0.2;        // side clearance between the cap's legs and the seat
hook_h = 1.2;          // height of the hooks' lead-in ramp
neck_in = 1.5;         // how far the seat overhangs its neck on each side
// Both parts print profile face down, so every thickness in their profiles is a
// vertical wall. The cap's top flexes to let the hooks spread (about 2% strain in PETG).
cap_wall = strong_wall;

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
  aw = tie_w + 2 * strong_wall;
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

// Clip profiles share a seat frame: u runs across the strip, v points away from the
// seat. The seat's top face is v = seat_t and its underside (the ledge) is v = 0.
seat_w = strip_w + strip_fit;
neck_h = hook_h + 0.5;            // room under the ledge for the hooks
clip_sole = strong_wall;          // the VHB mounting sole stays stiff and flat
clip_shoulder = cap_wall + snap_fit + 0.5;  // the wedge stays this far out from the seat
function seat_to_profile(p) = [p[0] * cos(clip_angle) - p[1] * sin(clip_angle),
                               p[0] * sin(clip_angle) + p[1] * cos(clip_angle)];
// Lift the tilted seat frame so the wedge's low shoulder corner sits on the sole.
seat_lift = clip_sole - seat_to_profile([-clip_shoulder, -neck_h])[1];

module clip_base_profile() {
  // Profile plane: x across the TV surface (-x points toward the TV edge), y away from
  // the TV. The mounting face is y = 0; the seat faces outward and backward. Below
  // v = -neck_h the base widens into a solid wedge, clear of everywhere the cap's legs go.
  pl = seat_to_profile([-clip_shoulder, -neck_h]) + [0, seat_lift];
  pr = seat_to_profile([seat_w + clip_shoulder, -neck_h]) + [0, seat_lift];
  translate([0, seat_lift]) rotate(clip_angle) {
    square([seat_w, seat_t]);
    translate([neck_in, -neck_h - eps]) square([seat_w - 2 * neck_in, neck_h + 2 * eps]);
  }
  polygon([pl, pr, [pr[0], 0], [pl[0], 0]]);
  translate([pl[0] - 1, 0]) square([pr[0] - pl[0] + 2, clip_sole]);
}

module clip_cap_profile() {
  c = snap_fit;
  hd = c + snap_over;                     // hook depth from the leg's inner face
  lip_v = seat_t + pcb_slot;              // lips' underside, on the PCB
  roof_v = lip_v + comp_clear;            // underside of the top between the lips
  top_v = roof_v + cap_wall;
  lx = strip_fit / 2 + lip_over;          // lip reach in from the seat edge
  difference() {
    translate([-c - cap_wall, -hook_h]) square([seat_w + 2 * (c + cap_wall), top_v + hook_h]);
    polygon([
      [-c, -hook_h - eps], [-c + hd, 0], [-c, 0], [-c, lip_v], [lx, lip_v], [lx, roof_v],
      [seat_w - lx, roof_v], [seat_w - lx, lip_v], [seat_w + c, lip_v], [seat_w + c, 0],
      [seat_w + c - hd, 0], [seat_w + c, -hook_h - eps]]);
  }
}

module clip_base() {
  linear_extrude(clip_len) clip_base_profile();
}

module clip_cap() {
  linear_extrude(cap_len) clip_cap_profile();
}

// In the base's coordinates, for previews only: the cap in place (or lifted off by
// `lift`), and a strip with two LEDs either side of it at 60 LED/m.
module in_seat_frame(z = 0) {
  translate([0, seat_lift, z]) rotate(clip_angle) children();
}

module clip_cap_seated(lift = 0) {
  in_seat_frame((clip_len - cap_len) / 2) translate([0, lift, 0]) clip_cap();
}

module strip_ghost() {
  pitch = 1000 / 60;
  in_seat_frame() {
    color("white") translate([strip_fit / 2, seat_t, -5]) cube([strip_w, pcb_slot, clip_len + 10]);
    for (z = [clip_len / 2 - pitch / 2, clip_len / 2 + pitch / 2])
      color("gold") translate([seat_w / 2 - 2.5, seat_t + pcb_slot, z - 2.5]) cube([5, 1.6, 5]);
  }
}

module clip_assembly(lift = 0) {
  clip_base();
  strip_ghost();
  color("orange", 0.8) clip_cap_seated(lift);
}

if (part == "base") base();
else if (part == "lid") lid();
else if (part == "clip_base") clip_base();
else if (part == "clip_cap") clip_cap();
else if (part == "clip_assembly") clip_assembly();
else {
  base();
  board_ghost();
  translate([0, 0, plate_t + inner_h + lid_t + 25]) mirror([0, 0, 1]) color("orange", 0.6) lid();
  translate([85, 0, 0]) clip_assembly();
}
