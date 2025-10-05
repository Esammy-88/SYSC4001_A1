/**
 *
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 *
 */


#include "interrupts.hpp"
#include <unordered_map> 

int main(int argc, char** argv) {

    //vectors is a C++ std::vector of strings that contain the address of the ISR
    //delays  is a C++ std::vector of ints that contain the delays of each device
    //the index of these elemens is the device number, starting from 0
    auto [vectors, delays] = parse_args(argc, argv);
    std::ifstream input_file(argv[1]);

    std::string trace;      //!< string to store single line of trace file
    std::string execution;  //!< string to accumulate the execution output

    int current_time = 100;           // simulation clock
    const int CONTEXT_SAVE = 10;    // different values for tests experiments
    const int ISR_ACTIVITY = 165;    // different values: 40–200 ms for analysis
    const int IRET = 1;  

    std::unordered_map<int, int> device_end_time; // device number time when its I/O completes
    //parse each line of the input trace file
    while(std::getline(input_file, trace)) {
        auto [activity, duration_intr] = parse_trace(trace);

        if (activity == "CPU") {
            // CPU burst
            execution += std::to_string(current_time) + ", " +
                         std::to_string(duration_intr) + ", CPU burst\n";
            current_time += duration_intr;
        }
        else if (activity == "SYSCALL") {
            int dev = duration_intr;

            // 1. Run interrupt boilerplate (switch → save context → vector lookup → load ISR)
            auto [log, new_time] = intr_boilerplate(current_time, dev, CONTEXT_SAVE, vectors);
            execution += log;
            current_time = new_time;

            // 2. Execute ISR body (device driver)
            execution += std::to_string(current_time) + ", " +
                         std::to_string(ISR_ACTIVITY) + ", execute ISR for device " +
                         std::to_string(dev) + "\n";
            current_time += ISR_ACTIVITY;

            // 3. IRET
            execution += std::to_string(current_time) + ", " +
                         std::to_string(IRET) + ", IRET\n";
            current_time += IRET;

            // 4. Record when this device’s I/O will complete (for END_IO)
            int delay = (dev < (int)delays.size()) ? delays[dev] : 100;
            device_end_time[dev] = current_time + delay;
        }
        else if (activity == "END_IO") {
            int dev = duration_intr;
            int delay = (dev < (int)delays.size()) ? delays[dev] : 100;

            // if device_end_time known, use that; else schedule now + delay
            int end_time = device_end_time.count(dev) ? device_end_time[dev] : current_time + delay;

            // 1. Run interrupt boilerplate again for END_IO
            auto [log, new_time] = intr_boilerplate(current_time, dev, CONTEXT_SAVE, vectors);
            execution += log;
            current_time = new_time;

            // 2. Log end-of-I/O handling
            execution += std::to_string(current_time) + ", " +
                         std::to_string(delay) + ", end of I/O " +
                         std::to_string(dev) + ": interrupt\n";
            current_time += delay;

            // 3. IRET
            execution += std::to_string(current_time) + ", " +
                         std::to_string(IRET) + ", IRET\n";
            current_time += IRET;
        }
        else {
            // Unknown activity — ignore
            execution += std::to_string(current_time) + ", 0, Unknown activity: " +
                         activity + "\n";
        }


    }

    input_file.close();

    write_output(execution);

    return 0;
}
