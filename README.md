This is my coursework submission for CSC8502: Advanced Graphics For Games. It consists of a planet scene created using OpenGL and C++, highlighting a number of rendering, lighting and post processing features which were implemented.
<br/><br/>
Youtube Video Showcase
https://youtu.be/P9bqlQVvOtg

<br/>
Key Binds <br/>
•	WASD - move camera <br/>
•	Shift, Space - move camera up/down <br/>
•	C - restart automatic camera track <br/>
•	V - manual camera control <br/>

•	Z - switch to/from space scene <br/>

•	P - enable bloom <br/>
•	O - enable blur <br/>
•	I - disable bloom/blur <br/>
•	N - toggle night vision on/off <br/>

<br/><br/>

Screenshots <br/><br/>

 ![image](https://github.com/JoshF02/8502_Coursework/assets/95030736/3d0e0c86-636f-42ee-874d-55440d2224db)

The scene contains a landscape as well as multiple meshes and textures, including a skeletally animated crab character, all rendered using a scene graph. There is a space skybox, and a water shader which provides environment mapping reflections of the skybox. All meshes generate real-time shadows based on the position of the sun, which orbits the scene and provides light as part of a day/night cycle. There are additional point lights which contribute to the scene lighting using forward shading. Below you can see the scene at night.

 <br/><br/>
 ![image](https://github.com/JoshF02/8502_Coursework/assets/95030736/c44ae81d-a303-42a9-bc37-df232d07f229)

<br/><br/>
![image](https://github.com/JoshF02/8502_Coursework/assets/95030736/26f77dc2-9729-4da0-a769-8371bb2d1f8a)

Multiple post processing effects were implemented – blur (above), bloom (below), and a ‘night vision’ effect which brightens the scene and tints it green. The blur and bloom effects use the same Gaussian blur technique; for bloom the bright areas of the frame are extracted and only these are blurred to create a glowing visual effect. 

<br/><br/>
 ![image](https://github.com/JoshF02/8502_Coursework/assets/95030736/32b95091-6f3f-4fb8-8717-875891c594ff)

 <br/><br/>
![image](https://github.com/JoshF02/8502_Coursework/assets/95030736/09603c7b-9ffa-4076-a31c-68283091c63f)

The ‘night vision’ effect was created within a fragment shader by applying a high gamma value to brighten the scene, and by scaling down the red and blue colour channels to tint the frame green.

<br/><br/>
 ![image](https://github.com/JoshF02/8502_Coursework/assets/95030736/04b4f5a0-105c-408e-bde9-9806036baf0f)

By pressing ‘Z’ you can transition to a view of the planet from space, which is lit using deferred rendering of multiple point lights – lights on the surface move and glow, while the sun orbits the planet and also contributes to the lighting.





<br/><br/>
Unity Assets Used <br/>
•	Free Rocks asset pack by Triplebrick - https://assetstore.unity.com/packages/3d/environments/landscapes/free-rocks-19288 <br/>
•	Insectoid Crab Monster by LB3D - https://assetstore.unity.com/packages/3d/characters/insectoid-crab-monster-lurker-of-the-shores-20-animations-107223 <br/>
•	Hi-Rez Sci-Fi Spaceships Creator Free Sample by Ebal Studios - https://assetstore.unity.com/packages/3d/vehicles/space/hi-rez-sci-fi-spaceships-creator-free-sample-153363 <br/>
•	Heightmap Generator by Manuel Iuliani - https://assetstore.unity.com/packages/tools/modeling/heightmap-generator-260294 <br/>

Textures Used <br/>
•	Sun and moon textures from https://www.solarsystemscope.com/textures/ <br/>
•	Space skybox from https://opengameart.org/content/space-skyboxes-0 <br/>
•	Terrain texture from https://polyhaven.com/a/rock_boulder_dry <br/>
•	Terrain texture from https://www.manytextures.com/texture/272/muddy-terrain/ <br/>
