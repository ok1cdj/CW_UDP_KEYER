//----------------------------------------------------------------------------------
//
// Morse paddle project using D2F microswitches, version 4 - large
// Part: Base
// (c) 2019 Jan Uhrin, OM2JU
//
//
$fn = 32;    // Rounding parameter; for production set to high (e.g. 64)
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
//
module BASE () {
difference() {
union() {
// base plate
translate([0,-7.8+28.3,2.0/2]) rcube(Size=[20+8+0.3,10-0.6,2.0],b=0.6);
    
//
// 2x 2 sticks for D2F microswitches
$dd1 = 2.04;
$h   = 4;
translate([-12.5,17.5,$h-2.0])      cylinder(h=$h,r1=$dd1/2,r2=$dd1/2.2,center=true);
translate([-12.5,17.5+6.15,$h-2.0]) cylinder(h=$h,r1=$dd1/2,r2=$dd1/2.2,center=true);
//
translate([ 12.5,17.5,$h-2.0])      cylinder(h=$h,r1=$dd1/2,r2=$dd1/2.2,center=true);
translate([ 12.5,17.5+6.15,$h-2.0]) cylinder(h=$h,r1=$dd1/2,r2=$dd1/2.2,center=true);

}

// cut the corners so that base fits in rounded shell (part: Cover)
rotate ([0,0,45]) translate([8,-18.0,3]) cube(size=[30,8,6],center=true);
rotate ([0,0,45]) translate([-18.0,8,3]) cube(size=[8,30,6],center=true);
rotate ([0,0,45]) translate([48,21,3]) cube(size=[8,30,6],center=true);
rotate ([0,0,45]) translate([21,48,3]) cube(size=[30,8,6],center=true);

// cut in rear side for manipulation
//translate([0,45,3]) cube(size=[12,8.5,6],center=true);

// cut in right side for easy insertion with screwdriver
translate([-26,22,3]) cube(size=[12,7,6],center=true);

// fixing screw
translate([0.0,18.0,0]) cylinder(h=10,r1=3.3/2,r2=3.38/2,center=true);
translate([0.0,18.0,-2.5]) cylinder(h=10,r1=16.5/2,r2=1.58/2,center=true);


}



// support for fixing screw  - in middle
//difference () {
//translate([0.0,18.0,7.6/2+0.4]) cube(size=[4.0,6.0,7.6],center=true);
//rotate ([0,0,0]) 
//translate([0.0,18.0,5.5]) cylinder(h=10,r1=2.9/2,r2=2.98/2,center=true);

//rotate ([90,0,0]) 
//translate([2.0,4.0,-30]) cylinder(h=10,r1=1.2/2,r2=1.2/2,center=true);

//rotate ([90,0,0]) 
//translate([-2.0,4.0,-30]) cylinder(h=10,r1=1.2/2,r2=1.2/2,center=true);
//}




} // end module BASE

translate([0,-30,0])
BASE();