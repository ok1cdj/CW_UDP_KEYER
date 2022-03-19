//----------------------------------------------------------------------------------
//
// Morse paddle project using D2F microswitches, version 4 - large
// Part: Paddles
// (c) 2019 Jan Uhrin, OM2JU
//
//
$fn = 40; // Rounding parameter; for production set to high (e.g. 40) 
//
//----------------------------------------------------------------------------------
//
// Rounded primitives for openscad
// (c) 2013 Wouter Robers 

// Syntax example for a rounded block
//translate([-15,0,10]) rcube([20,20,20],2);

// Syntax example for a rounded cylinder
//translate([15,0,10]) rcylinder(r1=15,r2=10,h=20,b=2);

module rcube(Size=[20,20,20],b=2)
{hull(){for(x=[-(Size[0]/2-b),(Size[0]/2-b)]){for(y=[-(Size[1]/2-b),(Size[1]/2-b)]){for(z=[-(Size[2]/2-b),(Size[2]/2-b)]){ translate([x,y,z]) sphere(b);}}}}}


module rcylinder(r1=10,r2=10,h=10,b=2)
{translate([0,0,-h/2]) hull(){rotate_extrude() translate([r1-b,b,0]) circle(r = b); rotate_extrude() translate([r2-b, h-b, 0]) circle(r = b);}}
    
//----------------------------------------------------------------------------------

$dd = 4.96;
 
module paddle(rear_x = 0.6+6.2/4) {
difference () {
union() {

translate([0.6,-3.5,5.0/2])    rcube(Size=[5,23.5,5.0],b=1.2);             // paddle
rotate ([30,0,0]) translate([0.6,-10.5,11])  rcube(Size=[5,4,8.0],b=1.2);  // paddle
translate([0.6,-15.5,7.0])     sphere(r=5.6);                              // paddle tentacle


translate([0.6,9.1,12.0/2])          rcube(Size=[8.8,9.0,12.0],b=1.2);     // middle part
translate([rear_x,16.2,6.0/2])       rcube(Size=[6.0/2,22,6.0],b=1.2);     // rear part
translate([rear_x,12/2+16.5,6.0/2])  rcube(Size=[6.8/2,10,6.0],b=1.2);     // rear part
}
translate([0.7,9.0,2.5])   cylinder(h=20,r1=$dd/2,r2=$dd/2,center=true);   // hole in middle
translate([0.7,9.0,0.4])   cylinder(h=5,r1=$dd,r2=$dd/4,center=true);      // conical cut 
translate([0.7,9.0,11.7])  cylinder(h=5,r1=$dd/4,r2=$dd,center=true);      // conical cut
}
}



difference() {
union() {
// two paddles
translate([-5.5,0,0])  paddle(rear_x = 0.6+1.55-2);
translate([+5.5,0,0])  paddle(rear_x = 0.6-1.55+2);
}
translate([0.6,-16,12.0/2])     cube(size=[4.3,10.0,12.0],center=true);
}