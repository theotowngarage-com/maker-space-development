import uasyncio
import machine

# Define the pins for the stepper motor
# The pins are defined as follows:
#   STEPPER_DIR: Direction pin for the stepper motor
#   STEPPER_STEP: Step pin for the stepper motor

OPEN_STEPS = 3 * 1600
STEPPER_DIRECTION_PIN = 18
STEPPER_STEP_PIN = 19
MOTOR_ENABLE_PIN = 5
STEP_TIME = 2 / 1600

ENCODER_IN_PIN = 21
OPEN_MIN_COUNT = 50

class AsyncEncoder:
    def __init__(self, pin):
        self.pin = machine.Pin(pin, machine.Pin.IN)
        self.count = 0
        self.prev_state = self.pin.value()
        self.thread = uasyncio.create_task(self.__thread())

    def __update(self):
        state = self.pin.value()
        if(state != self.prev_state):
            self.count += 1
            self.prev_state = state
    
    async def __thread(self):
        while True:
            self.__update()
            await uasyncio.sleep_ms(1)

    def read(self):
        return self.count
    
    def reset(self):
        self.count = 0

class AsyncStepper:
    def __init__(self, direction_pin, step_pin, enable_pin):
        self.direction_pin = machine.Pin(direction_pin, machine.Pin.OUT)
        self.step_pin = machine.Pin(step_pin, machine.Pin.OUT)
        self.enable_pin = machine.Pin(enable_pin, machine.Pin.OUT)
        self.enable_pin.value(1)

    async def move(self, direction, steps):
        self.enable_pin.value(0)
        self.direction_pin.value(direction)
        for i in range(steps):
            if(self.step_pin.value() == 1):
                self.step_pin.value(0)
            else:
                self.step_pin.value(1)
            await uasyncio.sleep(STEP_TIME)
        self.enable_pin.value(1)

class AsyncMotorizedLock:
    def __init__(self, stepper, encoder):
        self.stepper = stepper
        self.encoder = encoder

    async def open_lock(self):
        self.encoder.reset()
        await self.stepper.move(1, OPEN_STEPS)
        print("Encoder count: {}".format(self.encoder.read()))
        if(self.encoder.read() < OPEN_MIN_COUNT):
            return False
        return True

    async def close_lock(self):
        self.encoder.reset()
        await self.stepper.move(0, OPEN_STEPS)
        print("Encoder count: {}".format(self.encoder.read()))
        if(self.encoder.read() < OPEN_MIN_COUNT):
            return False
        return True

async def main():
    print("Starting lock test")
    # Create an instance of the stepper class
    encoder = AsyncEncoder(ENCODER_IN_PIN)
    stepper = AsyncStepper(STEPPER_DIRECTION_PIN, STEPPER_STEP_PIN, MOTOR_ENABLE_PIN)
    lock = AsyncMotorizedLock(stepper, encoder)
    
    print("Starting lock test 2")

    await uasyncio.sleep(1)
    print("Opening lock")
    # Open the lock
    if not await lock.open_lock():
        print("Lock did not open")
    else:
        print("Lock opened")

    await uasyncio.sleep(5)
    # Close the lock
    print("Closing lock")
    if not await lock.close_lock():
        print("Lock did not close")
    else:
        print("Lock closed")

    print("Done")

print("Starting main")
uasyncio.get_event_loop().run_until_complete(main())
