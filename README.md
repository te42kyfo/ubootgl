# Uboats!

## Description

Uboats puts the fun into CFD with the following steps:

1. Take a 2D Fluid Simulation
2. Put small uboats into the flow
3. Add torpedos
4. ...
5. Profit!

## Screenshots

![Screenshot from 2022-05-21 15-20-28](https://user-images.githubusercontent.com/3269202/169653565-a7664d2e-dd4c-4eff-b296-3df5877dd4b7.png)

Complete and numerically sound, highly optimized fluid simulation using a Finite Difference Discretization with a Semi-Lagrange Advection scheme. 

![Screenshot from 2022-05-21 13-36-16](https://user-images.githubusercontent.com/3269202/169653568-f1979090-5e4e-4023-ba21-26a2e10a6bbe.png)

Uboats! features 1-4 player split screen modes.

![Screenshot from 2022-05-21 13-31-38](https://user-images.githubusercontent.com/3269202/169653570-ee19a40d-e9b5-4c49-aa94-6b3c5ca7a112.png)

Watch how the explosions make craters in the fully destructible terrain, creates a shower of particles and a pressure wave.

## Build instructions

### Dependencies

Uboats! has been developed on Arch Linux using various GCC versions. Building has been tested on Ubuntu and Windows.

Uboats! requires the following libraries to build:

- GLM
- SDL2
- GLEW

Then simply type ```make```in the terminal. 

### Hardware requirements

Uboats! requires an OpenGL4+ capable GPU. Performance has been optimized for an Intel HD620 integrated GPU and an i7-8550U quad core CPU. Any modern IGP and quad core CPU should work. 

Please report errors and problems with building the game and running on other graphics drivers!

