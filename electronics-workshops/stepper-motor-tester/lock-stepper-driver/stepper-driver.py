import uasyncio
import machine

# Define the pins for the stepper motor
# The pins are defined as follows:
#   STEPPER_DIR: Direction pin for the stepper motor
#   STEPPER_STEP: Step pin for the stepper motor

OPEN_STEPS = 3 * 1600
STEPPER_DIR = 18
STEPPER_STEP = 19
STEP_TIME = 2 / 1600

ENCODER_IN = 21
OPEN_MIN_COUNT = 50

asyc_loop = uasyncio.get_event_loop()

class Encoder:
    def __init__(self, pin):
        self.pin = machine.Pin(pin, machine.Pin.IN)
        self.count = 0
        self.prev_state = self.pin.value()
        self.thread = asyc_loop.create_task(self._thread())

    def update(self):
        state = self.pin.value()
        if(state != self.prev_state):
            self.count += 1
            self.prev_state = state
    
    async def _thread(self):
        while True:
            self.update()
            await uasyncio.sleep_ms(1)

    def read(self):
        return self.count

class Stepper:
    def __init__(self, dir, step):
        self.dir = machine.Pin(dir, machine.Pin.OUT)
        self.step = machine.Pin(step, machine.Pin.OUT)

    async def move(self, direction, steps):
        self.dir.value(direction)
        for i in range(steps):
            if(self.step.value() == 1):
                self.step.value(0)
            else:
                self.step.value(1)
            await uasyncio.sleep(STEP_TIME)

def open_lock(stepper):
    encoder = Encoder(ENCODER_IN)
    asyc_loop.run_until_complete(stepper.move(1, OPEN_STEPS))
    print("Encoder count: {}".format(encoder.read()))
    if(encoder.read() < OPEN_MIN_COUNT):
        print("Lock did not open fully")

def close_lock(stepper):
    encoder = Encoder(ENCODER_IN)
    asyc_loop.run_until_complete(stepper.move(0, OPEN_STEPS))
    print("Encoder count: {}".format(encoder.read()))
    if(encoder.read() < OPEN_MIN_COUNT):
        print("Lock did not close fully")

async def main():
    print("Starting lock test")
    # Create an instance of the stepper class
    stepper = Stepper(STEPPER_DIR, STEPPER_STEP)
    
    print("Starting lock test 2")

    await uasyncio.sleep(1)
    print("Opening lock")
    # Open the lock
    open_lock(stepper)
    print("Lock opened")

    await uasyncio.sleep(5)
    # Close the lock
    print("Closing lock")
    close_lock(stepper)

    print("Done")

print("Starting main")
asyc_loop.run_until_complete(main())
