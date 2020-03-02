/** Text LCD module "SB1602E" class library
 *
 *  @author  Tedd OKANO & Masato YAMANISHI
 *  @version 2.01
 *  @date    05-Jan-2015
 *
 *  SB1602E is an I2C based low voltage text LCD panel (based Sitronix ST7032 chip)
 *  The module by StrawberryLinux
 *  http://strawberry-linux.com/catalog/items?code=27002 (Online shop page (Japanese))
 *  http://strawberry-linux.com/pub/ST7032i.pdf (datasheet of the chip)
 *
 *  This is a library to operate this module easy.
 *
 *  Released under the Apache 2 license License
 *
 *  revision history (class lib name was "TextLCD_SB1602E")
 *    revision 1.0  22-Jan-2010   a. 1st release
 *    revision 1.1  23-Jan-2010   a. class name has been changed from lcd_SB1602E to TextLCD_SB1602E
 *                                b. printf() added
 *                                c. copyright notice added
 *    revision 1.3  02-May-2014   a. puticon() added (for SB1602B) by Masato YAMANISHI san
 *    revision 2.0  20-Oct-2014   a. class name is changed and published as "SB1602E"
 *                                b. re-written for better usability
 */

#ifndef        MBED_SB1602E
#define        MBED_SB1602E

#include    <stdarg.h>
#include    "mbed.h"
#include    "SB1602E.h"


/** PCA9629A class
 *
 *  This is a driver code for the PCA9629A stepper motor controller.
 *  This class provides interface for PCA9629A operation and accessing its registers.
 *  Detail information is available on next URL.
 *    http://www.nxp.com/products/interface_and_connectivity/i2c/i2c_bus_controller_and_bridge_ics/PCA9629APW.html
 *
 *  Example:
 *  @code
 *  #include "mbed.h"
 *  #include "SB1602E.h"
 *  
 *  SB1602E lcd(  p9, p10 );  //  SDA, SCL
 *  
 *  int main() {
 *      lcd.printf( 0, "Hello world!" );    //  line# (0 or 1), string
 *      lcd.printf( 1, "pi = %.6f", 3.14159265 );
 *  }
 *  @endcode
 */
class SB1602E
{
public:

    /** Create a SB1602E instance which is connected to specified I2C pins with specified address
     *
     * @param I2C_sda I2C-bus SDA pin
     * @param I2C_scl I2C-bus SCL pin
     * @param init_massage string to initialize the LCD
     */
    SB1602E( PinName I2C_sda, PinName I2C_scl, char *init_massage = NULL );

    /** Create a PCA9629A instance connected to specified I2C pins with specified address
     *
     * @param I2C object (instance)
     * @param init_massage string to initialize the LCD
     */
    SB1602E( I2C &i2c_, char *init_massage = NULL );
    
    /** Destractor
     */
    ~SB1602E();

    /** Printf
     *
     *  pirntf function with line number. 
     *  it can be used like
     *
     *  lcd.printf( 0, "Hello world!" );
     *  lcd.printf( 1, "pi = %.6f", 3.14159265 );
     *
     * @param line line# (0 for upper, 1 for lower)
     * @param format following parameters are compatible to stdout's printf
     */
    void printf( char line, char *format, ... );

    /** Put character : "putc()"
     *
     * @param line line# (0 for upper, 1 for lower)
     * @param c character code
     */
    void putc( char line, char c );

    /** Put string : "puts()"
     *
     * @param line line# (0 for upper, 1 for lower)
     * @param s pointer to a string data
     */
    void puts( char line, char *s );

    /** Put character into specified screen position
     *
     * @param c character code
     * @param x horizontal character position on the LCD
     * @param y vertical character position on the LCD
     */
    void putcxy( char c, char x, char y );

    /** Clear the LCD
     */
    void clear( void );

    /** Contrast adjustment
     *
     * @param contrast value (from 0x00 to 0x3E)
     */
    void contrast( char contrast );

    /** Put a custom character given as bitmap data
     *
     * @param c_code character code
     * @param cg pointer to bitmap data (array of 8 bytes)
     * @param x horizontal character position on the LCD
     * @param y vertical character position on the LCD
     */
    void put_custom_char( char c_code, const char *cg, char x, char y );

    /** Set CGRAM (set custom bitmap as a character)
     *
     * @param c_code character code
     * @param cg pointer to bitmap data (array of 8 bytes)
     */
    void set_CGRAM( char char_code, const char* cg );

    /** Set CGRAM (set custom bitmap as a character)
     *
     * @param c_code character code
     * @param v bitmap data (5 bit pattern in this variable are copied to all row of a character bitmap)
     */
    void set_CGRAM( char char_code, char v );

    /** Icon operation (for SB1602B)
     *
     * @param flg bitpattern to choose ICON
     */
    void puticon( unsigned short flg );
    
private:
    char    curs[2];
    void    init( char *init_massage );
    void    clear_lest_of_line( char line );
    int     lcd_write( char first, char second );
    int     lcd_command( char command );
    int     lcd_data( char data );
    I2C     *i2c_p;
    I2C     &i2c;
    char    i2c_addr;

private:
    typedef enum {
#ifdef INIT_VALUE_DATASHEET_ORIGINAL
        Comm_FunctionSet_Normal      = 0x38,
        Comm_FunctionSet_Extended    = 0x39,
        Comm_InternalOscFrequency    = 0x14,
        Comm_ContrastSet             = 0x78,
        Comm_PwrIconContrast         = 0x5E,
        Comm_FollowerCtrl            = 0x6A,
        Comm_DisplayOnOff            = 0x0C,
        Comm_ClearDisplay            = 0x01,
        Comm_EntryModeSet            = 0x06,
#else
        Comm_FunctionSet_Normal      = 0x38,
        Comm_FunctionSet_Extended    = 0x39,
        Comm_InternalOscFrequency    = 0x14,
        Comm_ContrastSet             = 0x70,
        Comm_PwrIconContrast         = 0x5C,
        Comm_FollowerCtrl            = 0x60,
        Comm_DisplayOnOff            = 0x0C,
        Comm_ClearDisplay            = 0x01,
        Comm_EntryModeSet            = 0x04,
        Comm_ReturnHome              = 0x02,
#endif
        Comm_SetCGRAM                = 0x40
    } _commands;
    
    typedef enum {
        MaxCharsInALine              = 0x10, //    buffer depth for one line (no scroll function used)
        COMMAND                      = 0x00,
        DATA                         = 0x40
    } _constants;
}
;

#endif








