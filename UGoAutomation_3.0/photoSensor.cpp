#include "global.h"
#include "photoSensor.h"

extern "C" {
    unsigned long calibrate_time;
    void photo_pin_init()
    {
        calibrate_time = millis();
        
    }
    int photo_pin(int signal_pin) {
        // variables:
        int sensorValue = 0;         // the sensor value
        int sensorMin = 1023;        // minimum sensor value
        int sensorMax = 0;           // maximum sensor value
        
        //calibrate during the first five seconds
        while (calibrate_time + 500 < millis()) {
            sensorValue = analogRead(signal_pin);

            // record the maximum sensor value
            if (sensorValue > sensorMax) {
              sensorMax = sensorValue + 5;
            }

            // record the minimum sensor value
            if (sensorValue < sensorMin) {
                if((sensorValue - 5) > 0)
                    sensorMin = sensorValue-3;
                else
                    sensorMin = sensorValue+2;
            }
            calibrate_time = millis();
        }
        
         LOG_PRINT(LOGGER_VERBOSE, "sensorMax: %d", sensorMax);
         LOG_PRINT(LOGGER_VERBOSE, "sensorMin: %d", sensorMin);
         LOG_PRINT(LOGGER_VERBOSE, "sensorValue: %d", analogRead(signal_pin));

        // read the sensor:
        sensorValue = analogRead(signal_pin);

        // apply the calibration to the sensor reading
        sensorValue = map(sensorValue, sensorMin, sensorMax, 0, 255);

        // in case the sensor value is outside the range seen during calibration
        sensorValue = constrain(sensorValue, 0, 255);

        return sensorValue;
        // int value = analogRead(signal_pin);
        // return value;
    }
}
