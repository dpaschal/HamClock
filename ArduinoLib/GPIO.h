#ifndef _GPIO_H
#define _GPIO_H

/* Class to read and write the GPIO pins on linux or freebsd.
 * N.B. methods take GPIO number, not header pin number.
 * This is a Meyers Singleton, it can not be instantiated, use GPIO::getGPIO() to gain access.
 *
 * data sheet: https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2835/BCM2835-ARM-Peripherals.pdf
 * errata: https://elinux.org/BCM2835_datasheet_errata
 */

#include <stdint.h>
#include <pthread.h>

#include "Arduino.h"

/* Error callback typedef for error reporting */
typedef void (*GPIO_ErrorCallback)(const char *message);

#if defined(_GPIO_FREEBSD)
#include <sys/types.h>
#include <libgpio.h>
#endif

#if defined(_GPIO_LIBGPIOD)
#include <gpiod.h>
#endif

class GPIO {

    public:

        static GPIO& getGPIO(void);
        bool isReady(void);
        void setAsInput(uint8_t p);
        void setAsOutput(uint8_t p);
        void setHi(uint8_t p);
        void setHiLo (uint8_t p, bool hi);
        void setLo(uint8_t p);
        bool readPin (uint8_t p);
        void setErrorHandler(GPIO_ErrorCallback handler);

#if __cplusplus > 199711L

        // enforce no copy or move, only possible in c++11
        GPIO(const GPIO&) = delete;             // Copy ctor
        GPIO(GPIO&&) = delete;                  // Move ctor
        GPIO& operator=(const GPIO&) = delete;  // Copy assignment
        GPIO& operator=(GPIO&&) = delete;       // Move assignment

#endif


    private:

        GPIO(void);

#if defined(_GPIO_LINUX)

        bool ready;
        volatile uint32_t *gbase;
        inline uint32_t GPIO_SEL_MASK (uint8_t p, uint32_t m) {
            return (m<<(3*(p%10)));
        }
        pthread_mutex_t lock;
        bool mapGPIOAddress(char ynot[]);

#elif defined(_GPIO_FREEBSD)

        bool ready;
        pthread_mutex_t lock;
        gpio_handle_t handle;

#elif defined(_GPIO_LIBGPIOD)

        bool ready;
        pthread_rwlock_t lock;                          // RWlock for better concurrency
        struct gpiod_chip *chip;
        struct gpiod_line_request *inputRequest;        // Separate input request
        struct gpiod_line_request *outputRequest;       // Separate output request
        struct gpiod_request_config *inputConfig;       // Reusable input config
        struct gpiod_request_config *outputConfig;      // Reusable output config

        /* Pin registry - track which pins need what configuration */
        static const uint8_t MAX_GPIO_PINS = 64;
        struct {
            bool configured;
            bool isOutput;
        } pinRegistry[MAX_GPIO_PINS];

        /* Pin state cache - avoid repeated reads */
        uint64_t pinStateCache;                         // Bitmap of pin states
        uint64_t pinOutputCache;                        // Which pins are outputs
        bool cacheValid;                                // Is cache up to date?

        /* Error handling */
        GPIO_ErrorCallback errorHandler;

        /* Internal optimization methods */
        void reconfigureRequests(void);                 // Batch reconfiguration
        void reportError(const char *message);          // Report error via callback
        void invalidateCache(void);                     // Mark cache as stale
        void updateCache(void);                         // Refresh cache from hardware

#endif

};

#endif // _GPIO_H
