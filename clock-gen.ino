const int DEBOUNCE_DELAY = 20;
const int ADVANCE_DURATION = 20;

const int BI_PIN = 8;
const int MONO_PIN = 6;
const int FRQ_PIN = 3;
const int OUT_PIN = 2;
const int HALT_PIN = 13;
const int DELAYS[4] = {1000, 500, 100, 40}; // 60, 120, 600, 1500 HZ

int delay_index = 0;
int astable = 1;

// bi, mono, frq
int intermediate[3] = {LOW, LOW, LOW};
unsigned long last_tick[3] = {0, 0, 0};
int triggered[3] = {0, 0, 0};

int out_write_type = LOW;
unsigned long out_last_tick = 0;

void read_input(int pin)
{
	int index;
	void (*action)();
	switch (pin) {
		case BI_PIN: index = 0; action = &toggle; break;
		case MONO_PIN: index = 1; action = &single_advance; break;
		case FRQ_PIN: index = 2; action = &change_delay; break;
	}

	unsigned long now = millis();
	int read = digitalRead(pin);

  // debounce switch input
	if (intermediate[index] != read) {
		intermediate[index] = read;
		last_tick[index] = now;
	} else if (intermediate[index] == HIGH && (now - last_tick[index]) >= DEBOUNCE_DELAY && !triggered[index]) {
		triggered[index] = 1;
		action();
	}

	if (intermediate[index] == LOW)
		triggered[index] = 0;
}

void toggle()
{
	if (astable)
		astable = 0;
	else
		astable = 1;
}

// monostable
void single_advance()
{
	unsigned long now = millis();

	if ((now - last_tick[1]) < ADVANCE_DURATION || out_write_type != LOW)
		return;

	out_write_type = HIGH;
	digitalWrite(OUT_PIN, out_write_type);

	last_tick[1] = now;

}

// monostable pulse
void advance_off()
{
	unsigned long now = millis();

	if ((now - last_tick[1]) < ADVANCE_DURATION || out_write_type != HIGH)
		return;

	out_write_type = LOW;
	digitalWrite(OUT_PIN, out_write_type);

	last_tick[1] = now;
}

// astable, change frequency
void change_delay()
{
	int array_length = *(&DELAYS + 1) - DELAYS;

	++delay_index;
	if (delay_index == array_length)
		delay_index = 0;
}

void pulse(int pin, int delay, int *last_write_type, unsigned long *last_tick)
{
	unsigned long now = millis();

	if (*last_write_type == HIGH && (now - *last_tick) >= ADVANCE_DURATION) {
		*last_write_type = LOW;
	} else if (*last_write_type == LOW && (now - *last_tick) >= (delay - ADVANCE_DURATION)) {
		*last_write_type = HIGH;
	} else {
		return;
	}

	digitalWrite(pin, *last_write_type);
	*last_tick = now;
}

void setup()
{
	pinMode(HALT_PIN, INPUT);
	pinMode(BI_PIN, INPUT);
	pinMode(MONO_PIN, INPUT);
	pinMode(FRQ_PIN, INPUT);
	pinMode(OUT_PIN, OUTPUT);
}

void loop()
{
	if (digitalRead(HALT_PIN))
		return;

	read_input(BI_PIN);

	if (astable) {
		read_input(FRQ_PIN);
		pulse(OUT_PIN, DELAYS[delay_index], &out_write_type, &out_last_tick);
	} else if (!astable) {
		read_input(MONO_PIN);
		advance_off();
	}
}
