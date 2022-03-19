//----------------------------------------------------------------------------------
//
// Morse paddle project using D2F microswitches, version 4 - large
// Part: Cover
// (c) 2019 Jan Uhrin, OM2JU
//
//
$fn = 64;    // Rounding parameter; for production set to high (e.g. 40) 
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

difference() {
// difference of two rounded cubes, creates shell like object
translate([0,0,40/2])   rcube(Size=[46+1.6,67+2,40],b=9);
translate([0,0,60/2+1]) rcube(Size=[46-1.6,67,60],b=9);  

// big cube from top to limit height
translate([0,0,5+10+85]) cube(size=[150,150,150],center=true);  

// Pushbuttons on TTG Board
translate([16.6/2,-27,0])   cylinder(h=10,r1=2.0,r2=2.0,center=true);
translate([-16.6/2,-27,0])  cylinder(h=10,r1=2.0,r2=2.0,center=true);

translate([16.6/2,-27,3.15])  rcylinder(r1=2.7,r2=2.7,h=4,b=0.1);
translate([-16.6/2,-27,3.15]) rcylinder(r1=2.7,r2=2.7,h=4,b=0.1);

    
// Cutout for display
translate([0,-1,0]) cube(size=[16,29,20],center=true);  

// front side opening for paddles
translate([0,25,5+12.0]) cube(size=[16.4,20,18],center=true);  
rotate([0,90,90]) translate([-5-8.4,5.4,30.0])  cylinder(h=10,r1=6.8,r2=6.8,center=true);  
rotate([0,90,90]) translate([-5-8.4,-5.4,30.0]) cylinder(h=10,r1=6.8,r2=6.8,center=true);  

// rear side hole cable
translate([0,-25,1+(6.0/2)]) cube(size=[12,45,6.0],center=true);  
//rotate([0,90,90]) translate([-13.0,0.0,-30.0]) cylinder(h=20,r1=5.8,r2=5.8,center=true);  

// cut from inside as guide for base plate which will be inserted
translate([0,0,5+17.4])   cube(size=[38+7.8,55.0,2],center=true);


}

// Holding structure for TTGO board
difference() {
 // Rectangles
 union() {
   translate([+25/2+1.0,-24.7,3.5]) cube(size=[3,5,6],center=true);  
   translate([-25/2-1.1,-24.7,3.5]) cube(size=[3,5,6],center=true);  

   translate([+25/2+1.0,+1,3.5]) cube(size=[3,5,6],center=true);  
   translate([-25/2-1.1,+1,3.5]) cube(size=[3,5,6],center=true);  

   translate([+25/2+1.0,+20,3.5]) cube(size=[3,5,6],center=true);  
   translate([-25/2-1.1,+20,3.5]) cube(size=[3,5,6],center=true);  
 }
 
 // Cutout
 translate([0,-4,4.5]) cube(size=[25.5,54,1.4],center=true);  

// Holes - idea is to insert needle into rectangles to bend them
// during insertion of module
translate([+27.2/2,20,6])  cylinder(h=10,r1=0.7,r2=0.7,center=true);
translate([-27.2/2,20,6])  cylinder(h=10,r1=0.7,r2=0.7,center=true);

translate([+27.2/2,1,6])  cylinder(h=10,r1=0.7,r2=0.7,center=true);
translate([-27.2/2,1,6])  cylinder(h=10,r1=0.7,r2=0.7,center=true);

translate([+27.2/2,-25,6])  cylinder(h=10,r1=0.7,r2=0.7,center=true);
translate([-27.2/2,-25,6])  cylinder(h=10,r1=0.7,r2=0.7,center=true);

}


// Stop wall for board
translate([0,+22.5,3.5]) cube(size=[5,3,4.0],center=true);  


// Button-1
translate([0,4,0.25]) rcylinder(r1=2.25,r2=2.25,h=0.5,b=0.1);
translate([0,4,1.5]) rcylinder(r1=1.8,r2=1.8,h=3,b=0.6);
// Button-2
translate([0,-4,0.25]) rcylinder(r1=2.25,r2=2.25,h=0.5,b=0.1);
translate([0,-4,1.5]) rcylinder(r1=1.8,r2=1.8,h=3,b=0.6);

