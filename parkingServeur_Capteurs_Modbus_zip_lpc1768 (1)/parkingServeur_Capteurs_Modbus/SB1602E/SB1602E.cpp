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

#include    <stdarg.h>
#include    "mbed.h"
#include    "SB1602E.h"


SB1602E::SB1602E( PinName I2C_sda, PinName I2C_scl, char *init_massage ) : i2c_p( new I2C( I2C_sda, I2C_scl ) ), i2c( *i2c_p )
{
    init( init_massage );
}

SB1602E::SB1602E( I2C &i2c_, char *init_massage ) : i2c_p( NULL ), i2c( i2c_ )
{
    init( init_massage );
}

SB1602E::~SB1602E()
{
    if ( NULL != i2c_p )
        delete  i2c_p;
}

#define     DEFAULT_CONTRAST    0x35

void SB1602E::init( char *init_massage )
{
    const char init_seq0[]  = {
        Comm_FunctionSet_Normal,
        Comm_ReturnHome,             //    This may be required to reset the scroll function
        Comm_FunctionSet_Extended,
        Comm_InternalOscFrequency,
        Comm_ContrastSet            | ( DEFAULT_CONTRAST       & 0xF),
        Comm_PwrIconContrast        | ((DEFAULT_CONTRAST >> 4) & 0x3),
        Comm_FollowerCtrl           | 0x0A,
    };
    const char init_seq1[]  = {
        Comm_DisplayOnOff,
        Comm_ClearDisplay,
        Comm_EntryModeSet,
    };
    
    i2c_addr    = 0x7C;

    wait( 0.04 );    //    interval after hardware reset

    for ( int i = 0; i < sizeof( init_seq0 ); i++ ) {
        lcd_command( init_seq0[ i ] );
        wait( 30e-6 );
    }

    wait( 0.2 );

    for ( int i = 0; i < sizeof( init_seq1 ); i++ ) {
        lcd_command( init_seq1[ i ] );
        wait( 2e-3 );
    }

    set_CGRAM( 7, '\x1F' );

    curs[ 0 ]    = 0;
    curs[ 1 ]    = 0;

    if ( init_massage )
    {
        puts( 0, init_massage );
        curs[ 0 ]    = 0;
    }
}

void SB1602E::printf( char line, char *format, ... )
{
    char    s[ 32 ];
    va_list args;

    va_start( args, format );
    vsnprintf( s, 32, format, args );
    va_end( args );

    puts( line, s );
}

void SB1602E::putc( char line, char c )
{
    if ( (c == '\n') || (c == '\r') ) {
        clear_lest_of_line( line );
        curs[ line ]    = 0;
        return;
    }

    putcxy( c, curs[ line ]++, line );
}

void SB1602E::puts( char line, char *s )
{
    while ( char c    = *s++ )
        putc( line, c );
}

void SB1602E::putcxy( char c, char x, char y )
{
    const char    Comm_SetDDRAMAddress        = 0x80;
    const char    DDRAMAddress_Ofst[]         = { 0x00, 0x40 };

    if ( (x >= MaxCharsInALine) || (y >= 2) )
        return;

    lcd_command( (Comm_SetDDRAMAddress | DDRAMAddress_Ofst[ y ]) + x );
    lcd_data( c );
}

void SB1602E::clear( void )
{
    lcd_command( Comm_ClearDisplay );
    wait( 2e-3 );
    curs[ 0 ]    = 0;
    curs[ 1 ]    = 0;
}

void SB1602E::contrast( char contrast )
{
    lcd_command( Comm_FunctionSet_Extended );
    lcd_command( Comm_ContrastSet         |  (contrast     & 0x0f) );
    lcd_command( Comm_PwrIconContrast     | ((contrast>>4) & 0x03) );
    lcd_command( Comm_FunctionSet_Normal   );
}

void SB1602E::put_custom_char( char c_code, const char *cg, char x, char y )
{
    for ( int i = 0; i < 5; i++ ) {
        set_CGRAM( c_code, cg );
        putcxy( c_code, x, y );
    }
}

void SB1602E::set_CGRAM( char char_code, const char* cg )
{
    for ( int i = 0; i < 8; i++ ) {
        lcd_command( (Comm_SetCGRAM | (char_code << 3) | i) );
        lcd_data( *cg++ );
    }
}

void SB1602E::set_CGRAM( char char_code, char v )
{
    char    c[ 8 ];

    for ( int i = 0; i < 8; i++ )
        c[ i ]    = v;

    set_CGRAM( char_code, c );
}

void SB1602E::clear_lest_of_line( char line )
{
    for ( int i = curs[ line ]; i < MaxCharsInALine; i++ )
        putcxy( ' ', i, line );
}

int SB1602E::lcd_write( char first, char second )
{
    char cmd[2];

    cmd[ 0 ]    = first;
    cmd[ 1 ]    = second;

    return ( i2c.write( i2c_addr, cmd, 2 ) );

}

int SB1602E::lcd_command( char command )
{
    return ( lcd_write( COMMAND, command ) );
}

int SB1602E::lcd_data( char data )
{
    return ( lcd_write( DATA, data ) );
}

//  Following function has been imported from Masato YAMANISHI san's code.
//  Thank you!
//  http://developer.mbed.org/users/masato/code/TextLCD_SB1602E/file/39110c58e55c/TextLCD_SB1602E.h

const unsigned char icon_data[]= {
    // アイコンアドレス, 該当ビット
    0x00, 0x10, // 0b10000,
    0x02, 0x10, // 0b10000,
    0x04, 0x10, // 0b10000,
    0x06, 0x10, // 0b10000,

    0x07, 0x10, // 0b10000,
    0x07, 0x08, // 0b01000,
    0x09, 0x10, // 0b10000,
    0x0B, 0x10, // 0b10000,

    0x0D, 0x08, // 0b01000,
    0x0D, 0x04, // 0b00100,
    0x0D, 0x02, // 0b00010,
    0x0D, 0x10, // 0b10000,

    0x0F, 0x10, // 0b10000, // アンテナマーク
};

void SB1602E::puticon(unsigned short flg)
{
    static unsigned char icon_buff[16]; // アイコンの編集用
    unsigned char i;

    for(i=0; i<sizeof(icon_data)/2; i++) {
        if(flg & (0x1000>>i)) { // 該当ビットが立っていたら
            icon_buff[icon_data[i*2]] |= icon_data[i*2+1];  // バッファを立てます。
        } else {
            icon_buff[icon_data[i*2]] &= ~icon_data[i*2+1]; // バッファをクリアします。
        }
    }
    // 一括でLCDに書き込みます。
    for(i=0; i<16; i++) {
        lcd_command(Comm_FunctionSet_Extended); // 0b00111001); // コマンド
        lcd_command(Comm_SetCGRAM + i); // 0b01000000+i);       // アイコン領域のアドレスを設定
        lcd_data(icon_buff[i]); // アイコンデータ
    }
}






