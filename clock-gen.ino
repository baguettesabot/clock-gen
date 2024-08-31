const int DEBOUNCE_DELAY = 20;
const int ADVANCE_DURATION = 20;

const int BI_PIN = 8;
const int MONO_PIN = 6;
const int FRQ_PIN = 3;
const int OUT_PIN = 2;
const int HALT_PIN = 13;
const int DELAYS[5] = {500, 250, 100, 50, 20};

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

void single_advance()
{
	unsigned long now = millis();

	if ((now - last_tick[1]) < ADVANCE_DURATION || out_write_type != LOW)
		return;

	out_write_type = HIGH;
	digitalWrite(OUT_PIN, out_write_type);

	last_tick[1] = now;

}

void advance_off()
{
	unsigned long now = millis();

	if ((now - last_tick[1]) < ADVANCE_DURATION || out_write_type != HIGH)
		return;

	out_write_type = LOW;
	digitalWrite(OUT_PIN, out_write_type);

	last_tick[1] = now;
}

void change_delay()
{
	int array_length = *(&DELAYS + 1) - DELAYS;

	++delay_index;
	if (delay_index == array_length)
		delay_index = 0;
}

void pulse(int pin, int duration, int *last_write_type, unsigned long *last_tick)
{
	unsigned long now = millis();

	if ((now - *last_tick) < duration)
		return;

	switch (*last_write_type) {
		case LOW:
			*last_write_type = HIGH;
			break;
		default:
			*last_write_type = LOW;
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
