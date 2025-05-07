Group Members: Keegan Evans, Bohdi Norvell, Connor James, Aiden Coss
# Swamp Cooler Project - Function Breakdown
## Core System Functions

1. **System State Management**
   - Function to track and transition between states (DISABLED, IDLE, ERROR, RUNNING)
   - State transition logging functionality
   - LED management for state indication (RED, GREEN, BLUE, YELLOW)

2. **Register-Level I/O Operations**
   - Direct register manipulation for digital I/O (replacing pinMode, digitalRead, digitalWrite)
   - Custom ADC reading implementation (replacing analogRead)
   - Pin configuration for all connected components

3. **Timer and Interrupt Management**
   - Custom ISR implementation for the start button
   - Non-blocking timer implementation for 1-minute updates
   - Real-time tracking without using delay()

## Sensor Integration Functions

4. **Water Level Monitoring**
   - Water level sensor reading using ADC
   - Threshold detection and error state triggering
   - Water level status reporting

5. **Temperature & Humidity Monitoring**
   - DHT11 sensor interfacing (library allowed)
   - Temperature threshold detection
   - Regular updates to the LCD display

## Output Control Functions

6. **Fan Motor Control**
   - Fan activation/deactivation based on temperature
   - Custom motor control functions
   - State-based control logic

7. **Vent Position Control**
   - Stepper motor interface (library allowed)
   - User input processing for position control
   - Position tracking and reporting

## User Interface Functions

8. **LCD Display Management**
   - Display initialization and configuration (library allowed)
   - Status information display functions
   - Error message display

9. **Button Input Processing**
   - Start button handling (with ISR)
   - Stop button handling
   - Reset button handling for error recovery

## System Communication Functions

10. **Real-Time Clock Integration**
    - RTC initialization and time retrieval (library allowed)
    - Time-stamping for events
    - Time formatting for display

11. **Serial Communication**
    - Custom UART implementation
    - Event reporting to host computer
    - Debugging information transmission

## Integration and Testing Functions

12. **System Initialization**
    - Component setup sequence
    - Initial state configuration
    - System parameter initialization

13. **Main Loop Control**
    - State machine execution
    - Sensor polling and event handling
    - System timing management

## Work Distribution Strategy

Here's how you might distribute these functions among 4 team members:

### Team Member 1: Core System & State Management
- System state machine implementation
- Register-level I/O operations
- Main loop coordination
- System initialization

### Team Member 2: Sensor Integration
- Water level monitoring implementation
- Temperature & humidity sensor integration
- Threshold detection logic
- Error detection

### Team Member 3: Output & Motor Control
- Fan motor control implementation
- Vent position control with stepper motor
- LED status indicators
- Timer management

### Team Member 4: UI & Communication
- LCD display management
- Button input processing
- Real-time clock integration
- Serial communication implementation
