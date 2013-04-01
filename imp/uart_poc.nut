// UART Communication To Main/Remote Boards

// Globals
message2_outlet_id_length <- 8;
message2_state_length <- 2;
message2_length <- message2_outlet_id_length + message2_state_length + 2; // Add 2 for starting @ and ending \0

// Output ports to update DB and get status of all outlets
local set_outlet_state = OutputPort("outlet-state", "string");
local init_outlets = OutputPort("init-outlets", "string");

// UART Setup
hardware.uart1289.configure(2400, 8, PARITY_NONE, 1, NO_CTSRTS);

function pad_lsb_zeros(field, length) {
    local difference = length - field.len();
    local result = field;
    for(local i = 0; i < difference; ++i) {
        result = "0" + result;
    }
    return result; 
}

// Command Info
class CommandIn extends InputPort
{
    function set(v) { 
            server.log("\n\tv.board_outlet_number: " + v.board_outlet_number + "\n" +
                        "\tv.board_product_key: " + v.board_product_key + "\n" +
                        "\tv.state: " + v.state + "\n" +
                        "\tv.outlet_id: " + v.outlet_id
                        );
            // 26 bytes total == 20 hex
            hardware.uart1289.write("!" + 
                pad_lsb_zeros(v.board_product_key, 10) +
                pad_lsb_zeros(v.board_outlet_number, 4) +
                pad_lsb_zeros(v.state, 2) +
                pad_lsb_zeros(v.outlet_id, 8) +
                "\0");
    }
};


function state_check() {
    /* Echo Check */
    /*
    while(1) {
        local byte = hardware.uart1289.read();
        if(byte < 0) {
            break;
        }
        server.log(byte + " ");
    }*/
    local backend_message2_json = "";
    local message2_outlet_id = "";
    local message2_state = "";
    local bytes_read = 0;
    local message_type = '~';
    local byte = hardware.uart1289.read();
    if(byte >= 0) {
        bytes_read++;
        message_type = byte;
        if(message_type == '@'){
            while(bytes_read < message2_length) {
                byte = hardware.uart1289.read();
                if(byte >= 0) {
                    if(bytes_read <= message2_outlet_id_length) {
                        message2_outlet_id = message2_outlet_id + byte.tochar();
                    }else if(bytes_read <= message2_length -2){
                        message2_state = message2_state + byte.tochar();
                    }else{
                        if(byte != '\0'){
                            server.log("Error: Last byte was NOT a null, instead: " + byte);
                        }else{
                            server.log("Response message successful: " +
                                        "\n\toutlet_id: " + message2_outlet_id + 
                                        "\n\tstate: " + message2_state);
                            //Send message to back end
                            backend_message2_json = @"{ ""outlet_id"": """ + message2_outlet_id + 
                                                    @""", ""state"": """ + message2_state + @""" }";
                            server.log("\n\n" + backend_message2_json);
                            set_outlet_state.set(backend_message2_json);
                        }   
                    }
                    bytes_read++;
                }else {
                    // SLEEP FOR .5 seconds
                    imp.sleep(0.1);
                }
                
            }            
        } else {
            server.log("Error: Unknown message type " + message_type + "\n");
        }
    }
    // Schedule function to be recalled
    imp.wakeup(3, state_check);
}

function outlets_init() {
    server.log("Initializing Outlets");
    init_outlets.set("dummy_data");
}

// Register with the server
imp.configure("UART-poc", [CommandIn()], [set_outlet_state, init_outlets]);
outlets_init(); // Get initial state of all outlets
state_check();
// End of code.
