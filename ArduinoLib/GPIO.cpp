/* simple GPIO access class for linux or freebsd suitable for HamClock.
 *
 * The class constructor will only be called once because it is not public and there is only one instance
 * within a static context. This is the hallmark of a Meyers Singleton. Can not be instantiated,
 * use GPIO::getGPIO() to gain access. The class is thread-safe, although of course the functioning of
 * the connected devices may not be.
 */

#include "GPIO.h"



#if defined(_GPIO_LINUX)

/************************************************************************************
 *
 * linux implementation
 *
 ************************************************************************************/


#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>

#include <bcm_host.h>


/* constructor
 * _GPIO_LINUX
 */
GPIO::GPIO()
{
        // prepare gpio access
        char ynot[1024];
        ready = mapGPIOAddress(ynot);
        if (ready) {
            // init lock for safe threaded access
            pthread_mutexattr_t lock_attr;
            pthread_mutexattr_init (&lock_attr);
            pthread_mutexattr_settype (&lock_attr, PTHREAD_MUTEX_RECURSIVE);
            pthread_mutex_init (&lock, &lock_attr);
        } else {
            // note why not
            printf ("GPIO: %s\n", ynot);
        }

}

/* return reference to the one shared instance
 * _GPIO_LINUX
 */
GPIO& GPIO::getGPIO()
{
        static GPIO the_one;         // the only instance, hence only one call to constructor
        return the_one;
}

/* return whether GPIO is suitable for use
 * _GPIO_LINUX
 */
bool GPIO::isReady()
{
    return (ready);
}

/* _GPIO_LINUX
 */
void GPIO::setAsInput(uint8_t p)
{
        if (!ready)
            return;

        pthread_mutex_lock(&lock);

        gbase[p/10] &= ~GPIO_SEL_MASK(p,7);

        // enable pullup -- BCM2835
        gbase[37] = 2;
        gbase[38+p/32] = 1UL << (p%32);
        gbase[37] = 0;
        gbase[38+p/32] = 0;

        // enable pullup -- BCM2711
        gbase[57+p/16] = (gbase[57+p/16] & ~(3UL << 2*(p%16))) | (1UL << 2*(p%16));

        pthread_mutex_unlock(&lock);
}

/* _GPIO_LINUX
 */
void GPIO::setAsOutput(uint8_t p)
{
        if (!ready)
            return;

        pthread_mutex_lock(&lock);
        gbase[p/10] = (gbase[p/10] & ~GPIO_SEL_MASK(p,7)) | GPIO_SEL_MASK(p,1);
        pthread_mutex_unlock(&lock);
}

/* _GPIO_LINUX
 */
void GPIO::setHi(uint8_t p)
{
        if (!ready)
            return;

        pthread_mutex_lock(&lock);
        gbase[7+p/32] = 1UL << (p%32);
        pthread_mutex_unlock(&lock);
}

/* _GPIO_LINUX
 */
void GPIO::setLo(uint8_t p)
{
        if (!ready)
            return;

        pthread_mutex_lock(&lock);
        gbase[10+p/32] = 1UL << (p%32);
        pthread_mutex_unlock(&lock);
}

/* _GPIO_LINUX
 */
void GPIO::setHiLo (uint8_t p, bool hi)
{
        if (!ready)
            return;

        if (hi)
            setHi (p);
        else
            setLo (p);
}

/* _GPIO_LINUX
 */
bool GPIO::readPin (uint8_t p)
{
        if (!ready)
            return(false);

        pthread_mutex_lock(&lock);
        bool hi = (gbase[13+p/32] & (1UL<<(p%32))) != 0;
        pthread_mutex_unlock(&lock);
        return (hi);
}

/* set gbase so it points to the physical address of the GPIO controller.
 * return true if ok, else false with brief excuse in ynot[].
 * _GPIO_LINUX
 */
bool GPIO::mapGPIOAddress(char ynot[])
{
        // access kernel physical address
        const char gpiofilefile[] = "/dev/gpiomem";
        int fd = open (gpiofilefile, O_RDWR|O_SYNC);
        if (fd < 0) {
            sprintf (ynot, "%s: %s", gpiofilefile, strerror(errno));
            return (false);
        }

        /* mmap access */
        gbase = (uint32_t *) mmap(NULL, 0xB4, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

        // fd not needed after setting up mmap
        close(fd);

        // check for error, leave gbase 0 if so
        if (gbase == MAP_FAILED) {
            gbase = NULL;
            sprintf (ynot, "mmap: %s", strerror(errno));
            return (false);
        }

        // worked
        return (true);
}


#elif defined(_GPIO_FREEBSD)


/************************************************************************************
 *
 * freebsd implementation
 *
 ************************************************************************************/



/* prepare class, set ready if useable.
 * _GPIO_FREEBSD
 */
GPIO::GPIO()
{
        handle = gpio_open(0);

        if (handle == GPIO_INVALID_HANDLE) {
            printf ("GPIO open failed\n");
            ready = false;
        } else {
            ready = true;
            // init lock for safe threaded access
            pthread_mutexattr_t lock_attr;
            pthread_mutexattr_init (&lock_attr);
            pthread_mutexattr_settype (&lock_attr, PTHREAD_MUTEX_RECURSIVE);
            pthread_mutex_init (&lock, &lock_attr);
        }
}

/* return reference to the one shared instance
 * _GPIO_FREEBSD
 */
GPIO& GPIO::getGPIO()
{
        static GPIO the_one;         // the only instance, hence only one call to constructor

    // #define _GPIO_LIST_PINS
    #ifdef _GPIO_LIST_PINS
        gpio_config_t *config;
        int n_pins = gpio_pin_list (the_one.handle, &config);
        for (int i = 0; i < n_pins; i++)
            printf ("pin %d flags %x\n", config[i].g_pin, config[i].g_flags);
        free (config);
    #endif // _TEST_GPIO

        return the_one;
}

/* return whether GPIO is suitable for use
 * _GPIO_FREEBSD
 */
bool GPIO::isReady()
{
        return (ready);
}

/* _GPIO_FREEBSD
 */
void GPIO::setAsInput (uint8_t p)
{
        pthread_mutex_lock(&lock);

        // must set input and pullup atomically
        gpio_config_t cfg;
        memset (&cfg, 0, sizeof(cfg));
        cfg.g_pin = p;
        cfg.g_flags = GPIO_PIN_INPUT | GPIO_PIN_PULLUP;
        gpio_pin_set_flags (handle, &cfg);

        pthread_mutex_unlock(&lock);
}

void GPIO::setAsOutput (uint8_t p)
{
        pthread_mutex_lock(&lock);
        gpio_pin_output (handle, p);
        pthread_mutex_unlock(&lock);
}

void GPIO::setHi (uint8_t p)
{
        pthread_mutex_lock(&lock);
        gpio_pin_high (handle, p);
        pthread_mutex_unlock(&lock);
}

void GPIO::setLo(uint8_t p)
{
        pthread_mutex_lock(&lock);
        gpio_pin_low (handle, p);
        pthread_mutex_unlock(&lock);
}

void GPIO::setHiLo (uint8_t p, bool hi)
{
        pthread_mutex_lock(&lock);
        gpio_pin_set (handle, p, hi != 0);
        pthread_mutex_unlock(&lock);
}

bool GPIO::readPin (uint8_t p)
{
        pthread_mutex_lock(&lock);
        bool s = gpio_pin_get (handle, p) == GPIO_VALUE_HIGH;
        pthread_mutex_unlock(&lock);
        return (s);
}

void GPIO::setErrorHandler(GPIO_ErrorCallback handler)
{
        // FreeBSD implementation doesn't use error callbacks
        // This is a no-op for compatibility with optimized version
}


#elif defined(_GPIO_LIBGPIOD)


/************************************************************************************
 *
 * Linux libgpiod v2 implementation (Debian Trixie and newer)
 *
 ************************************************************************************/


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <gpiod.h>


/* constructor
 * _GPIO_LIBGPIOD
 */
GPIO::GPIO()
{
        chip = NULL;
        inputRequest = NULL;
        outputRequest = NULL;
        inputConfig = NULL;
        outputConfig = NULL;
        ready = false;
        errorHandler = NULL;
        pinStateCache = 0;
        pinOutputCache = 0;
        cacheValid = false;

        // Initialize pin registry
        for (int i = 0; i < MAX_GPIO_PINS; i++) {
            pinRegistry[i].configured = false;
            pinRegistry[i].isOutput = false;
        }

        // Try to open the default GPIO chip
        chip = gpiod_chip_open("/dev/gpiochip0");
        if (!chip) {
            reportError("Unable to open /dev/gpiochip0");
            return;
        }

        ready = true;

        // init read-write lock for better concurrency
        pthread_rwlockattr_t lock_attr;
        pthread_rwlockattr_init(&lock_attr);
        pthread_rwlock_init(&lock, &lock_attr);
        pthread_rwlockattr_destroy(&lock_attr);

        // Create reusable config objects
        inputConfig = gpiod_request_config_new();
        outputConfig = gpiod_request_config_new();

        if (!inputConfig || !outputConfig) {
            reportError("Failed to allocate request configs");
            ready = false;
        }
}


/* return reference to the one shared instance
 * _GPIO_LIBGPIOD
 */
GPIO& GPIO::getGPIO()
{
        static GPIO the_one;
        return the_one;
}


/* return whether GPIO is suitable for use
 * _GPIO_LIBGPIOD
 */
bool GPIO::isReady()
{
        return (ready);
}


/* _GPIO_LIBGPIOD - Optimized: Mark pin for input without immediate reconfiguration
 */
void GPIO::setAsInput(uint8_t p)
{
        if (!ready || p >= MAX_GPIO_PINS)
            return;

        pthread_rwlock_wrlock(&lock);
        pinRegistry[p].configured = true;
        pinRegistry[p].isOutput = false;
        invalidateCache();
        pthread_rwlock_unlock(&lock);

        // Deferred reconfiguration happens on next access
}


/* _GPIO_LIBGPIOD - Optimized: Mark pin for output without immediate reconfiguration
 */
void GPIO::setAsOutput(uint8_t p)
{
        if (!ready || p >= MAX_GPIO_PINS)
            return;

        pthread_rwlock_wrlock(&lock);
        pinRegistry[p].configured = true;
        pinRegistry[p].isOutput = true;
        invalidateCache();
        pthread_rwlock_unlock(&lock);

        // Deferred reconfiguration happens on next access
}


/* _GPIO_LIBGPIOD - Optimized: Set pin HIGH with minimal lock overhead
 */
void GPIO::setHi(uint8_t p)
{
        if (!ready || p >= MAX_GPIO_PINS)
            return;

        pthread_rwlock_rdlock(&lock);

        // Reconfigure if needed (detects dirty requests)
        if (pinRegistry[p].configured && !outputRequest) {
            pthread_rwlock_unlock(&lock);
            pthread_rwlock_wrlock(&lock);
            reconfigureRequests();
        }

        if (outputRequest) {
            gpiod_line_request_set_value(outputRequest, p, 1);
            invalidateCache();
        }

        pthread_rwlock_unlock(&lock);
}


/* _GPIO_LIBGPIOD - Optimized: Set pin LOW with minimal lock overhead
 */
void GPIO::setLo(uint8_t p)
{
        if (!ready || p >= MAX_GPIO_PINS)
            return;

        pthread_rwlock_rdlock(&lock);

        // Reconfigure if needed
        if (pinRegistry[p].configured && !outputRequest) {
            pthread_rwlock_unlock(&lock);
            pthread_rwlock_wrlock(&lock);
            reconfigureRequests();
        }

        if (outputRequest) {
            gpiod_line_request_set_value(outputRequest, p, 0);
            invalidateCache();
        }

        pthread_rwlock_unlock(&lock);
}


/* _GPIO_LIBGPIOD - Optimized: Set pin to HIGH or LOW state
 */
void GPIO::setHiLo(uint8_t p, bool hi)
{
        if (hi)
            setHi(p);
        else
            setLo(p);
}


/* _GPIO_LIBGPIOD - Optimized: Read pin state with software cache
 */
bool GPIO::readPin(uint8_t p)
{
        if (!ready || p >= MAX_GPIO_PINS)
            return false;

        pthread_rwlock_rdlock(&lock);

        // Ensure requests are configured
        if (pinRegistry[p].configured && !inputRequest && !outputRequest) {
            pthread_rwlock_unlock(&lock);
            pthread_rwlock_wrlock(&lock);
            reconfigureRequests();
        }

        // Use cache if valid
        if (cacheValid && pinRegistry[p].configured) {
            bool val;
            if (pinRegistry[p].isOutput) {
                val = (pinOutputCache & (1ULL << p)) != 0;
            } else {
                val = (pinStateCache & (1ULL << p)) != 0;
            }
            pthread_rwlock_unlock(&lock);
            return val;
        }

        // Cache miss - read from hardware
        bool result = false;
        struct gpiod_line_request *req = pinRegistry[p].isOutput ? outputRequest : inputRequest;
        if (req) {
            int val = gpiod_line_request_get_value(req, p);
            result = (val == 1);

            // Update cache
            if (result) {
                pinStateCache |= (1ULL << p);
            } else {
                pinStateCache &= ~(1ULL << p);
            }
        }

        pthread_rwlock_unlock(&lock);
        return result;
}


/* _GPIO_LIBGPIOD - Batch reconfiguration: single syscall for all pins
 * This is called lazily when GPIO operations are first attempted
 */
void GPIO::reconfigureRequests(void)
{
        // Release old requests
        if (inputRequest) {
            gpiod_line_request_release(inputRequest);
            inputRequest = NULL;
        }
        if (outputRequest) {
            gpiod_line_request_release(outputRequest);
            outputRequest = NULL;
        }

        // Rebuild configs with all configured pins
        gpiod_request_config_clear(inputConfig);
        gpiod_request_config_clear(outputConfig);

        gpiod_request_config_set_consumer(inputConfig, "hamclock-input");
        gpiod_request_config_set_consumer(outputConfig, "hamclock-output");

        // Add all configured pins to appropriate request
        for (int i = 0; i < MAX_GPIO_PINS; i++) {
            if (!pinRegistry[i].configured)
                continue;

            if (pinRegistry[i].isOutput) {
                gpiod_request_config_add_line_by_offset(outputConfig, i);
            } else {
                gpiod_request_config_add_line_by_offset(inputConfig, i);
            }
        }

        // Create input request if needed (single syscall for all input pins)
        struct gpiod_line_config *inputLine = gpiod_line_config_new();
        if (inputLine) {
            gpiod_request_config_set_line_config(inputConfig, inputLine);
            inputRequest = gpiod_chip_request_lines(chip, inputConfig);
            if (!inputRequest) {
                reportError("Failed to configure input pins");
            }
            gpiod_line_config_free(inputLine);
        }

        // Create output request if needed (single syscall for all output pins)
        struct gpiod_line_config *outputLine = gpiod_line_config_new();
        if (outputLine) {
            gpiod_line_config_set_direction_output(outputLine);
            gpiod_request_config_set_line_config(outputConfig, outputLine);
            outputRequest = gpiod_chip_request_lines(chip, outputConfig);
            if (!outputRequest) {
                reportError("Failed to configure output pins");
            }
            gpiod_line_config_free(outputLine);
        }

        cacheValid = false;
}


/* _GPIO_LIBGPIOD - Report error via callback if set
 */
void GPIO::reportError(const char *message)
{
        if (errorHandler) {
            errorHandler(message);
        } else {
            printf("GPIO Error: %s\n", message);
        }
}


/* _GPIO_LIBGPIOD - Mark cache as invalid
 */
void GPIO::invalidateCache(void)
{
        cacheValid = false;
        pinStateCache = 0;
        pinOutputCache = 0;
}


/* _GPIO_LIBGPIOD - Refresh cache from hardware
 */
void GPIO::updateCache(void)
{
        if (!inputRequest && !outputRequest)
            return;

        pinStateCache = 0;
        pinOutputCache = 0;

        // Read all configured pins from hardware
        for (int i = 0; i < MAX_GPIO_PINS; i++) {
            if (!pinRegistry[i].configured)
                continue;

            struct gpiod_line_request *req = pinRegistry[i].isOutput ? outputRequest : inputRequest;
            if (req) {
                int val = gpiod_line_request_get_value(req, i);
                if (val == 1) {
                    pinStateCache |= (1ULL << i);
                    if (pinRegistry[i].isOutput) {
                        pinOutputCache |= (1ULL << i);
                    }
                }
            }
        }

        cacheValid = true;
}


/* Public: Set error callback for debugging
 */
void GPIO::setErrorHandler(GPIO_ErrorCallback handler)
{
        pthread_rwlock_wrlock(&lock);
        errorHandler = handler;
        pthread_rwlock_unlock(&lock);
}


#else


// dummy placeholders

GPIO::GPIO() { }
GPIO& GPIO::getGPIO()
{
        static GPIO the_one;         // the only instance, hence only one call to constructor
        return the_one;
}
void GPIO::setAsInput(uint8_t p) { } 
void GPIO::setAsOutput(uint8_t p) { }
void GPIO::setHi(uint8_t p) { }
void GPIO::setLo(uint8_t p) { }
void GPIO::setHiLo (uint8_t p, bool hi) { }
bool GPIO::readPin (uint8_t p) { return false; }
void GPIO::setErrorHandler(GPIO_ErrorCallback handler) { }

#endif
