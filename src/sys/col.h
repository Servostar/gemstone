
#ifndef COLORS_H_
#define COLORS_H_

#define ANSI_ENABLED  1
#define ASNI_DISABLED 0

// Common escape codes
// can be used to print colored text
extern char* RED;
extern char* YELLOW;
extern char* MAGENTA;
extern char* CYAN;
extern char* GREEN;
extern char* RESET;
extern char* BOLD;
extern char* FAINT;

/**
 * @brief Initialize global state
 */
void col_init(void);

/**
 * @brief Enable ANSI escape codes. This will set the correct escape codes to
 *        the global strings above.
 */
void enable_ansi_colors();

/**
 * @brief Disable ANSI escape codes. This will set all the above global strings
 * to be empty.
 */
void disable_ansi_colors();

/**
 * @brief Check if stdout may support ANSI escape codes.
 * @attention This function may report escape codes to be unavailable even if
 * they actually are.
 * @return ANSI_ENABLED if escape sequences are supported ASNI_DISABLED
 * otherwise
 */
[[nodiscard]]
int stdout_supports_ansi_esc();

#endif // COLORS_H_
