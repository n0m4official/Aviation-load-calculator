# Aviation Load Calculator

Aviation Load Calculator is a project designed to assist aviation professionals and enthusiasts in accurately calculating and managing aircraft load and balance. Proper load calculation ensures safe and efficient aircraft operations by preventing overloading and ensuring balance limits are maintained.

## Features

- **Weight and Balance Calculation:** Easily input aircraft weights, fuel, cargo, passengers, and calculate total weight and CG (center of gravity).
- **Custom Aircraft Profiles:** Add and manage different aircraft types with their specific loading configurations and limits.
- **Safety Checks:** Automatic alerts for out-of-limit conditions (overweight, out-of-balance).
- **User-Friendly Interface:** Simple data input and result visualization.

## Getting Started

### Prerequisites

- C++ compiler (e.g., GCC, Clang, MSVC)
- [Optional] CMake for build management

### Building the Project

1. Clone the repository:
   ```bash
   git clone https://github.com/n0m4official/Aviation-load-calculator.git
   cd Aviation-load-calculator
   ```

2. Compile the code:
   - Using g++:
     ```bash
     g++ -o LoadCalc_CPP LoadCalc_CPP.cpp
     ```
   - Or use your favorite IDE/build tool to open `LoadCalc_CPP.sln` (Visual Studio Solution).

3. Run the application:
   - On Linux/Mac:
     ```bash
     ./LoadCalc_CPP
     ```
   - On Windows:
     - Double-click `LoadCalc_CPP.exe` or run from terminal:
       ```cmd
       LoadCalc_CPP.exe
       ```

### Usage

- The application will read from the included databases:  
  - `aircraft_db.json` (for aircraft profiles)
  - `uld_db.json` (for Unit Load Devices)
- Follow the on-screen prompts to enter aircraft data.
- Review the calculated total weight and CG.
- Ensure all values are within safe operational limits.

### Notes

- Make sure `json.hpp`, `aircraft_db.json`, and `uld_db.json` are in the same directory as your executable (`LoadCalc_CPP.exe` or `LoadCalc_CPP`).
- If you wish to modify or add aircraft or ULD profiles, edit the respective JSON files.

## Project Structure

```
Aviation-load-calculator/
├── .gitattributes
├── .gitignore
├── LICENSE.txt
├── LoadCalc_CPP.cpp
├── LoadCalc_CPP.exe
├── LoadCalc_CPP.sln
├── LoadCalc_CPP.vcxproj
├── LoadCalc_CPP.vcxproj.filters
├── README.md
├── aircraft_db.json
├── json.hpp
└── uld_db.json
```

## Contributing


Contributions are welcome! Please open issues for feature requests or bugs, and feel free to submit pull requests.

---

## Known Issues

| Issue                     | Response                  |
|---------------------------|---------------------------|
| exe version doesn't read .json files | Aware of issue. Unsure of cause. Working on fix. |
| Creating custom aircraft causes infinite loop | Aware of issue. Unsure of cause. Working on fix. |
| Calculated position(s) not correct | This was developed as a passion project, not meant for industry use. See disclaimer below. |
| Number and/or location of positions on aircraft are not correct | Issue cause by how rendering works in terminal. Working on fix. |
| Program is buggy | Still a work in progress. Please be patient and understand that this is being developed as a passion project. |

---

## License

This project is open source and available under the [MIT License](LICENSE.txt).

## Author

- [n0m4official](https://github.com/n0m4official)

---

## Disclaimer 

This tool is for educational and reference purposes only. Always verify calculations and consult official aircraft documentation before flight.
