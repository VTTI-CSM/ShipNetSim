<h1 align="center">
  <a href="https://github.com/VTTI-CSM/ShipNetSim">
    <img src="https://github.com/user-attachments/assets/60d3ed30-01fa-4f21-832e-93d72749031b" alt="NeTrainSim"/>

  </a>
  <br/>
  ShipNetSim [Network Ships Simulator]
</h1>

<p align="center">
  <a href="">
    <img src="https://zenodo.org/badge/DOI/10.2139/ssrn.4377164.svg" alt="DOI">
  </a>
  <a href="https://www.gnu.org/licenses/gpl-3.0">
    <img src="https://img.shields.io/badge/License-GPLv3-blue.svg" alt="License: GNU GPL v3">
  </a>
  <a href="https://github.com/VTTI-CSM/ShipNetSim/releases">
    <img alt="GitHub tag (latest by date)" src="https://img.shields.io/github/v/tag/VTTI-CSM/ShipNetSim.svg?label=latest">
  </a>
  <img alt="GitHub All Releases" src="https://img.shields.io/github/downloads/VTTI-CSM/ShipNetSim/total.svg">
  <a href="">
    <img src="https://img.shields.io/badge/CLA-CLA%20Required-red" alt="CLA Required">
    <a href="https://cla-assistant.io/VTTI-CSM/ShipNetSim"><img src="https://cla-assistant.io/readme/badge/VTTI-CSM/ShipNetSim" alt="CLA assistant" /></a>
  </a>
</p>

<p align="center">
  <a href="https://github.com/VTTI-CSM/ShipNetSim/releases" target="_blank">Download ShipNetSim</a> |
  <a href="https://VTTI-CSM.github.io/ShipNetSim/" target="_blank">Documentation</a> |
</p>

# Ship Network Simulator (ShipNetSim)
ShipNetSim is an open-source simulation software designed to analyze ship energy consumption and emissions in extensive maritime networks. Built with a modular and adaptable structure using Qt6, it integrates propulsion-resistance models, real-time environmental data, and advanced path-finding algorithms for longitudinal vessel motion analysis.

## How to Cite

```bibtex
@INPROCEEDINGS{10733439,
  author={Aredah, Ahmed and Rakha, Hesham A.},
  booktitle={2024 IEEE International Conference on Smart Mobility (SM)}, 
  title={ShipNetSim: A Multi-Ship Simulator for Evaluating Longitudinal Motion, Energy Consumption, and Carbon Footprint of Ships}, 
  year={2024},
  volume={},
  number={},
  pages={116-121},
  keywords={Measurement;Energy consumption;Adaptation models;Carbon dioxide;Trajectory;Fuels;Marine vehicles;Greenhouse gases;Carbon footprint;ShipNetSim;Ships Large-Scale Simulation;Ships Longitudinal Motion;Energy Consumption;Environmental Footprint},
  doi={10.1109/SM63044.2024.10733439}}
```

## Features

- **Open Access**: ShipNetSim is freely accessible and openly modifiable to support community collaboration.
- **Modular and Flexible**: Ships of varying types, sizes, and fuel types can be analyzed, with scalability to support evolving policies and technological advancements.
- **Environmental Sensitivity**: Capable of simulating operational strategies like reduced speeds and alternate fueling to comply with International Maritime Organization (IMO) standards.
- **Cybersecurity Modeling**: Analyzes risks like GPS spoofing and network disruptions, simulating impact on ship navigation and energy efficiency.
- **Real-time Analytics**: Tracks energy consumption, emissions, and vessel dynamics for each ship, enabling data-driven decisions.
- **Adaptable Pathfinding**: Integrates visibility graphs and QuadTree indexing to optimize navigation and obstacle avoidance on real-world routes.

## Getting Started
Download the latest release version on the releases page.

## Prerequisites
ShipNetSim requires no additional setup, as all third-party dependencies are bundled within the installer.

## Installation

- Download the installer file.
- Double-click to open and follow the setup prompts. The default path is C:\Program Files\ShipNetSim, but this can be adjusted as needed.

## Running

### GUI Interface
- The GUI is still under developement. 

### Shell Interface
- Open a terminal or command prompt.

- Navigate to the installation folder:

```shell
cd "C:\Program Files\ShipNetSim"
Type ShipNetSim -h to view command options.
```

```shell
ShipNetSim.exe -s "path\to\ships\file"
```

## Collaborators

- **Ahmed Aredah, M.Sc.**: 
     - Ph.D. student, Dept. of Civil and Environmental Engineering, Virginia Tech  
     - M.Sc. Student, Dept. of Computer Science | Engineering, Virginia Tech  
     - Graduate Research Assistant at Virginia Tech Transportation Institute

- **Hesham A. Rakha, Ph.D. P.Eng., F.IEEE**: 
     - Samuel Reynolds Pritchard Professor of Engineering, Charles E. Via, Jr. Dept. of Civil and Environmental Engineering
     - Courtesy Professor, Bradley Department of Electrical and Computer Engineering
     - Director, Center for Sustainable Mobility at the Virginia Tech Transportation Institute
     - Fellow of Asia Pacific Artificial Intelligence Association
     - Fellow of the American Society of Civil Engineers
     - Fellow of the Canadian Academy of Engineering
     - Fellow of IEEE

## Contributing
Interested in contributing? Please see our CONTRIBUTING.md file for guidelines on how to participate.

## Contributors

<!-- ALL-CONTRIBUTORS-LIST:START - Do not remove or modify this section -->
<!-- prettier-ignore-start -->
<!-- markdownlint-disable -->
<table>
  <tbody>
    <tr>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/VTTI-CSM"><img src="https://avatars.githubusercontent.com/u/137198701?v=4?s=100" width="100px;" alt="Center for Sustainable Mobility (CSM) at Virginia Tech"/><br /><sub><b>Center for Sustainable Mobility (CSM) at Virginia Tech</b></sub></a><br /><a href="#projectManagement-VTTI-CSM" title="Project Management">📆</a></td>
    </tr>
  </tbody>
</table>

<!-- markdownlint-restore -->
<!-- prettier-ignore-end -->

<!-- ALL-CONTRIBUTORS-LIST:END


## License
ShipNetSim is licensed under GNU GPL v3. See the LICENSE file for more details.
