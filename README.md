# CS330-Final-Project

***What new design skills has your work on the project helped you to craft?***
Because of my completion of CS330-Comp Graphics and Visualization, I am now much more knowledgable in the mathematical equations used to construct primitive objects. I have learned how to construct equations for making a symmetrical hexadecagon prism, a ring torus, and a sphere.

***What design process did you follow for your project work?***
I applied the Graphics Pipeline to my OpenGL 3D Scene. Once I created the setup for the scene, I plotted my vertices in a mesh and passed them to the Vertex Shader. I then used an Element Buffer Object to create indices from my coordinates to show Shape Assmebly. I did not apply the Geometry Shader to this scene so continued to the Rasterization Stage to obtain fragments for the Fragment Shader to use. The Fragment Shader then applied the colors and textures involved in my 3D Scene. Finally, I set my correct alpha values and enabled a depth test to blend objects accordingly. 

***How could tactics from your design approach be applied in future work?***
One of the biggest advantages of adhering to a design process is that it breaks larger complex projects into smaller components that are usually easier to handle. I experienced this through the milestone assignments that I completed in CS330, each contributing a margin of progression towards my final project. I have done this with other Computer Science classes as well where I implement an SDLC that is best suited for a particular project. As I get more experienced, the complexity of challenges I will face will continue to grow so being able to chunk assignments will be useful not only in completing tasks, but understanding them for future use.

***What new development strategies did you use while working on your 3D scene?***
I had a couple new strategies I used for the 3D Scene. This project is probably where I have implemented the most debugging and error-solving techniques to date. I used Print Debugging to determine the vertices of my sphere as it was created through a loop so I could provide the correct texture and normal coordinates to them as I had initial issues with this. I used reactive debugging to troubleshoot as errors and bugs were encountered. I also used preemptive debugging techniques like setting unit tests to see if shader information was retrieved successfully. 

***How did iteration factor into your development?***
Iteration allowed me to refine smaller chunks of my code easier than if I had to manage the whole solution all at once. I was able to catch bugs much more easily as I was developing in increments and making executables of my project every week to assure functionality. During later milestones, this not only allowed me to apply new knowledge but refine and repeat earlier techniques to create a better project.

***How has your approach to developing code evolved throughout the milestones, which led you to the project’s completion?***
Like with similar projects up until now, each has left an impression on different ways I can develop my code. This project has evolved my expertise on procedural programming as there are steps that must be taken sequentially to create a successful OpenGL project. An example of this is creating the GLFW Window instantiation before creating the window object.

***How do computational graphics and visualizations give you new knowledge and skills that can be applied in your future educational pathway?***
In terms of education, I have become much more familiar with Microsoft Visual Studio and setting up directories and linkers correctly for a complex project. I hope to use this for future complex projects that require a lot of components to run successfully. 

***How do computational graphics and visualizations give you new knowledge and skills that can be applied in your future professional pathway?***
Although I did not have a deep understanding of Graphic Coding and thus did not heavily inspire to follow that path in Computer Science, I feel that these skills learned during this class have given me another opportunity to expand my toolset and thus my employment potential.
