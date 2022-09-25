
#include <homekit/homekit.h>
#include <homekit/characteristics.h>

#define DEFAULT_COOLER_TEMPERATURE 27
#define MIN_COOLER_TEMPERATURE 16
#define MAX_COOLER_TEMPERATURE 30

typedef struct ac_state_struct {
    bool active;
    int targetTemperature;
    bool swing;
    int rotationSpeed;
} ac_state_t;

ac_state_t AC = {
    .active = false,
    .rotationSpeed = 1,
    .swing = false,
    .targetTemperature = DEFAULT_COOLER_TEMPERATURE
};


void ac_disp_ir();

void ac_disp_on_setter(const homekit_value_t value);

homekit_characteristic_t cha_ac_disp_on = HOMEKIT_CHARACTERISTIC_(ON, false, .setter = ac_disp_on_setter);

void ac_disp_on_setter(const homekit_value_t value) {
	bool on = value.bool_value;
	cha_ac_disp_on.value.bool_value = on;
    ac_disp_ir();
}

void ac_ir_send(bool on, uint8_t temp, uint8_t fan, bool swing);

void ac_ir_command() {
    ac_ir_send(AC.active, AC.targetTemperature, AC.rotationSpeed, AC.swing);
    printf("AC: status = %d, temp = %d, rot = %d, swing = %d\n", AC.active, AC.targetTemperature, AC.rotationSpeed, AC.swing);
}

void my_accessory_identify(homekit_value_t _value) {
    printf("accessory identify\n");
}

homekit_value_t ac_active_get() {
    return HOMEKIT_UINT8(AC.active);
};

void ac_active_set(homekit_value_t value);

homekit_characteristic_t ac_active = HOMEKIT_CHARACTERISTIC_(ACTIVE, 0, .getter = ac_active_get, .setter = ac_active_set);

void ac_active_set(homekit_value_t value) {
    ac_active.value = value;
    AC.active = value.bool_value;
    AC.swing = false; // Swing mode is always reset to false
    cha_ac_disp_on.value.bool_value = value.bool_value;
    homekit_characteristic_notify(&cha_ac_disp_on, cha_ac_disp_on.value);
    ac_ir_command();
};

homekit_characteristic_t current_temperature = HOMEKIT_CHARACTERISTIC_(CURRENT_TEMPERATURE, 26);

homekit_value_t ac_target_temperature_get() {
    return HOMEKIT_FLOAT((float)AC.targetTemperature);
};

void ac_target_temperature_set(homekit_value_t value);

homekit_characteristic_t target_temperature = HOMEKIT_CHARACTERISTIC_(
    COOLING_THRESHOLD_TEMPERATURE, DEFAULT_COOLER_TEMPERATURE,
    .min_value = (float[]){MIN_COOLER_TEMPERATURE},
    .max_value = (float[]){MAX_COOLER_TEMPERATURE}, 
    .min_step = (float[]){1},
    .getter = ac_target_temperature_get, .setter = ac_target_temperature_set
);

void ac_target_temperature_set(homekit_value_t value) {
    if (!AC.active) {
        printf("Can't change temperature while AC is off\n");
        homekit_characteristic_notify(&target_temperature, target_temperature.value);
        return;
    }

    AC.targetTemperature = (int)(value.float_value + 0.1);
    ac_ir_command();
}

homekit_characteristic_t units = HOMEKIT_CHARACTERISTIC_(TEMPERATURE_DISPLAY_UNITS, 0);

homekit_characteristic_t current_heater_cooler_state = HOMEKIT_CHARACTERISTIC_(CURRENT_HEATER_COOLER_STATE, 0);

homekit_characteristic_t target_heater_cooler_state = HOMEKIT_CHARACTERISTIC_(TARGET_HEATER_COOLER_STATE, 2,
    .valid_values = {
        .count = 2,
        .values = (uint8_t[]){2, 3}, /* AUTO = 0; HEAT = 1; COOL = 2; */
    }
);

homekit_value_t ac_speed_get() {
    return HOMEKIT_FLOAT((float)AC.rotationSpeed);
}

void ac_speed_set(homekit_value_t value);

homekit_characteristic_t ac_rotation_speed = HOMEKIT_CHARACTERISTIC_(ROTATION_SPEED, 1,
    .min_value = (float[]){0},
    .max_value = (float[]){4},
    .getter = ac_speed_get,
    .setter = ac_speed_set
);

void ac_speed_set(homekit_value_t value) {
    if (!AC.active) {
        printf("Can't change rotation speed while AC is off\n");
        homekit_characteristic_notify(&ac_rotation_speed, ac_rotation_speed.value);
        return;
    }

    AC.rotationSpeed = (int)(value.float_value + 0.1);
    ac_ir_command();
}

homekit_value_t ac_swing_get() {
    return HOMEKIT_UINT8(AC.swing);
}

void ac_swing_set(homekit_value_t value);

homekit_characteristic_t ac_swing_mode = HOMEKIT_CHARACTERISTIC_(SWING_MODE, 0,
    .getter = ac_swing_get,
    .setter = ac_swing_set
);

void ac_swing_set(homekit_value_t value) {
    AC.swing = value.bool_value;
    ac_swing_mode.value = value;
    ac_ir_command();
}

void light_rf_send();

void switch_rf_command(bool on) {
    printf("Light: %d\n", on);
    light_rf_send();
}

void light_on_setter(const homekit_value_t value);

homekit_characteristic_t cha_switch_on = HOMEKIT_CHARACTERISTIC_(ON, false, .setter = light_on_setter);

void light_on_setter(const homekit_value_t value) {
	bool on = value.bool_value;
	cha_switch_on.value.bool_value = on;
    switch_rf_command(on);
}

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id = 1, .category=homekit_accessory_category_bridge, .services=(homekit_service_t*[]) {
        // HAP section 8.17:
        // For a bridge accessory, only the primary HAP accessory object must contain this(INFORMATION) service.
        // But in my test,
        // the bridged accessories must contain an INFORMATION service,
        // otherwise the HomeKit will reject to pair.
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Smart Home"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "Arduino HomeKit"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "0123456"),
            HOMEKIT_CHARACTERISTIC(MODEL, "ESP8266"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, my_accessory_identify),
            NULL
        }),
        NULL
    }),
    HOMEKIT_ACCESSORY(.id = 2, .category = homekit_accessory_category_air_conditioner, .services=(homekit_service_t*[]) {
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
			HOMEKIT_CHARACTERISTIC(NAME, "Air Conditioner"),
			HOMEKIT_CHARACTERISTIC(IDENTIFY, my_accessory_identify),
			NULL
		}),
        HOMEKIT_SERVICE(HEATER_COOLER, .primary = true, .characteristics=(homekit_characteristic_t *[]) {
            &ac_active,
            &current_temperature,
            &target_temperature,
            &current_heater_cooler_state,
            &target_heater_cooler_state,
            &units,
            &ac_rotation_speed,
            &ac_swing_mode,
            NULL
        }),
        NULL
    }),
    HOMEKIT_ACCESSORY(.id = 3, .category = homekit_accessory_category_switch, .services=(homekit_service_t*[]) {
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
			HOMEKIT_CHARACTERISTIC(NAME, "AC Display"),
			HOMEKIT_CHARACTERISTIC(IDENTIFY, my_accessory_identify),
			NULL
		}),
		HOMEKIT_SERVICE(SWITCH, .primary=true, .characteristics=(homekit_characteristic_t*[]){
			&cha_ac_disp_on,
			NULL
		}),
        NULL
    }),
    HOMEKIT_ACCESSORY(.id = 4, .category = homekit_accessory_category_lightbulb, .services=(homekit_service_t*[]) {
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
			HOMEKIT_CHARACTERISTIC(NAME, "Light"),
			HOMEKIT_CHARACTERISTIC(IDENTIFY, my_accessory_identify),
			NULL
		}),
		HOMEKIT_SERVICE(SWITCH, .primary=true, .characteristics=(homekit_characteristic_t*[]){
			&cha_switch_on,
			NULL
		}),
        NULL
    }),
    NULL
};


homekit_server_config_t config = {
        .accessories = accessories,
        .password = "111-11-111"
};
