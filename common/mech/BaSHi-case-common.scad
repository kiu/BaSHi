$fn = $preview?6:360;

thick = 3;
tolerance = 0.1;

pcb_width = 23;

pcb_top = 15;
pcb_thick = 1.6;
pcb_bottom = 4;

cut_terminal_width = 17;

cut_top_pcb_offset = 31;
cut_top_pcb_height = 6;

pcb_border = 1.5;

box_inner_x = pcb_width + 2 * tolerance;
box_outer_x = box_inner_x + 2 * thick;

lip_width = thick / 2 - tolerance;
lip_height = 3;

// -------------------------------------------------------

b_box_inner_x_pcb = pcb_width - 2 * pcb_border + 2 * tolerance;

b_box_inner_z = pcb_bottom + pcb_thick + tolerance;
b_box_outer_z = b_box_inner_z + thick;

// --------------------------------------------------------

t_box_inner_z = pcb_top + tolerance - lip_height;
t_box_outer_z = t_box_inner_z + thick;

// --------------------------------------------------------

module case_top(pcb_height) {    
    box_inner_y = pcb_height + 2 * tolerance;
    box_outer_y = box_inner_y + 2 * thick;
    
    difference() {
        
        union() {
            // Initial box
            cube ([box_outer_x, box_outer_y, t_box_outer_z]);

            // Lip
            translate([thick - lip_width, thick - lip_width, t_box_outer_z])
                cube ([lip_width, box_outer_y - thick - lip_width, lip_height]);
            translate([box_outer_x - thick, thick - lip_width, t_box_outer_z])
                cube ([lip_width, box_outer_y - thick - lip_width, lip_height]);
            translate([thick - lip_width, thick - lip_width, t_box_outer_z])
                cube ([box_outer_x - 2 * thick  + 2 * lip_width, lip_width, lip_height]);
            translate([thick - lip_width, box_outer_y - thick, t_box_outer_z])
                cube ([box_outer_x - 2 * thick + 2 * lip_width, lip_width, lip_height]);
        }

        // Remove inner for pcb
        translate([box_outer_x/2 - b_box_inner_x_pcb/2, box_outer_y/2 - box_inner_y/2, thick])
            cube ([b_box_inner_x_pcb, box_inner_y, t_box_inner_z]);

        // Cutout inner for pcb
        translate([box_outer_x/2 - box_inner_x/2, thick + cut_top_pcb_offset, thick])
            cube ([box_inner_x, cut_top_pcb_height, t_box_inner_z]);

        // Remove Terminal        
        translate([box_outer_x/2 - cut_terminal_width/2, box_outer_y - thick, thick])
            cube ([cut_terminal_width, box_outer_y, t_box_inner_z + lip_height]);
    }
}

module case_bottom(pcb_height) {
    box_inner_y = pcb_height + 2 * tolerance;
    box_outer_y = box_inner_y + 2 * thick;
    
    difference() {
        
        union() {
            // Initial box
            cube ([box_outer_x, box_outer_y, b_box_outer_z]);
            
            // Lip
            translate([0, 0, b_box_outer_z])
                cube ([lip_width, box_outer_y, lip_height]);
            translate([box_outer_x - lip_width, 0, b_box_outer_z])
                cube ([lip_width, box_outer_y, lip_height]);
            translate([0, 0, b_box_outer_z])
                cube ([box_outer_x, lip_width, lip_height]);
            translate([0, box_outer_y - lip_width, b_box_outer_z])
                cube ([box_outer_x, lip_width, lip_height]);       
        }
        
        // Remove for pcb
        translate([box_outer_x/2 - box_inner_x/2, box_outer_y/2 - box_inner_y/2, b_box_outer_z - pcb_thick - tolerance])
            cube ([box_inner_x, box_inner_y, pcb_thick + tolerance]);

        // Cutout inner for pcb
        translate([box_outer_x/2 - box_inner_x/2, thick + cut_top_pcb_offset, thick])
            cube ([box_inner_x, cut_top_pcb_height, t_box_inner_z]);

        // Remove inner
        translate([box_outer_x/2 - b_box_inner_x_pcb/2, box_outer_y/2 - box_inner_y/2, thick])
            cube ([b_box_inner_x_pcb, box_inner_y, b_box_inner_z]);

        // Remove lip
        translate([box_outer_x/2 - cut_terminal_width/2, box_outer_y - thick, thick + b_box_inner_z])
            cube ([cut_terminal_width, box_outer_y, lip_height]);
        
        translate([box_outer_x/2 - b_box_inner_x_pcb/2, box_outer_y/2 - box_inner_y/2, thick])
            cube ([b_box_inner_x_pcb, box_inner_y, b_box_inner_z]);

    }
   
}
