#include <Arduino.h>

int main(void)
{
	init();

#if defined(USBCON)
	USBDevice.attach();
#endif
	
	setup();
    
	for (;;) {
		loop();
		if (serialEventRun) serialEventRun();
	}
        
	return 0;
}

#include <TaskAction.h>
#include <Wire.h>
#include <dht.h>
#include <BMP085.h>

#define I2C_ADDRESS 0x69

/*byte reg[] = {
    0x33,   // status
    0x00,   // outside temperature
    0x00,   // outside pressure
    0x00,   // inside temperature
    0x00,   // inside humidity
    0x00,   // inside luminance
    I2C_ADDRESS // id
  };*/

struct state {
  uint8_t  pins;
  int16_t  out_temperature;
  uint16_t out_pressure;
  int16_t  in_temperarure;
  uint8_t  in_humidity;
  uint16_t in_luminance;
  uint8_t  water;
} state;

#define IN_ALARM        7
#define IN_WATER_A      1
#define IN_LUMINANCE_A  6
#define OUT_ALARM       13

byte cmd[2];

dht dht11;
BMP085 bmp085;

byte get_state(void) {
  byte state = 0, mask = 4;
  for (byte i = 2; i < 8; ++i, mask += mask) if (digitalRead(i)) state |= mask;
  return state;
}

void requestEvent(void) {
  Wire.write((const uint8_t *)&state, sizeof(state));
}

void receiveEvent(int nrcv) {
  Serial.print("RCV ");
  Serial.print(nrcv, DEC);
  Serial.print(" ");
  for (byte i = 0; i < nrcv; ++i) {
    byte c = Wire.read();
    Serial.print(c, HEX);
    if (i < sizeof(cmd)) cmd[i] = c;
  }
  Serial.println("");
}

void updateState(void) {
  state.pins = get_state();
  state.water = (1023 - analogRead(IN_WATER_A)) > 100;
  //
  char buf[128];
  sprintf(buf, "STATUS %X %d", state.pins, state.water);
  Serial.println(buf);
}
TaskAction updateStateTask(updateState, 1000, INFINITE_TICKS);

void updateWeather(void) {
  dht11.read11(10);
  state.in_temperarure = dht11.temperature;
  state.in_humidity = dht11.humidity;
  //
  int32_t x;
  bmp085.getTemperature(&x);
  state.out_temperature = x;
  bmp085.getPressure(&x);
  state.out_pressure = 3.0*x/40;
  //
  state.in_luminance = 1023 - analogRead(IN_LUMINANCE_A);
  //
  char buf[128];
  sprintf(buf, "WEATHER %d %u %d %u %u",
      state.out_temperature,
      state.out_pressure,
      state.in_temperarure,
      state.in_humidity,
      state.in_luminance
    );
  Serial.println(buf);
}
TaskAction updateWeatherTask(updateWeather, 5000, INFINITE_TICKS);

void setup(void) {
  Serial.begin(9600);
  //
  Wire.begin(I2C_ADDRESS);
  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);
  //
  delay(1000);
  bmp085.init(MODE_ULTRA_HIGHRES, 0, true);
  //
  pinMode(IN_ALARM, INPUT);
  pinMode(OUT_ALARM, OUTPUT);
}

void loop(void) {
  updateStateTask.tick();
  updateWeatherTask.tick();
  //
  digitalWrite(OUT_ALARM, digitalRead(IN_ALARM));
  //
  delay(100);
}
