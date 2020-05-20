library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.std_logic_arith.conv_integer;
use IEEE.numeric_std.all;

entity cpld_kbd is
	port
	(
			CLK	     : in std_logic;

         A           : in std_logic_vector(15 downto 8);     				-- address bus for kbd
         KB          : out std_logic_vector(4 downto 0) := "ZZZZZ";     -- data bus for kbd

         AVR_MOSI    : in std_logic;
         AVR_MISO    : out std_logic;
         AVR_SCK     : in std_logic;
			AVR_SS      : in std_logic;
			
			O_RESET		: out std_logic;
			O_TURBO	 	: out std_logic;
			O_MAGICK		: out std_logic;
			O_SPECIAL	: out std_logic;
			
			O_CLK			: out std_logic;
			O_TEST 		: out std_logic;
			O_DEBUG 		: out std_logic
	);
    end cpld_kbd;

architecture RTL of cpld_kbd is

	 -- keyboard state
	 signal kb_data : std_logic_vector(39 downto 0) := (others => '0'); -- 40 keys

	 -- additional signals
	 signal reset   : std_logic := '0';
	 signal turbo   : std_logic := '0';
	 signal magick  : std_logic := '0';
	 signal special : std_logic := '0';
	 
	 -- spi
	 signal spi_do_valid : std_logic := '0';
	 signal spi_do : std_logic_vector(15 downto 0);

begin

U_SPI: entity work.spi_slave
    generic map(
        N              => 16 -- 2 bytes (cmd + data)       
    )
    port map(
        clk_i          => CLK,
        spi_sck_i      => AVR_SCK,
        spi_ssel_i     => AVR_SS,
        spi_mosi_i     => AVR_MOSI,
        spi_miso_o     => AVR_MISO,

        di_req_o       => open,
        di_i           => open,
        wren_i         => '0',
        do_valid_o     => spi_do_valid,
        do_o           => spi_do,

        do_transfer_o  => open,
        wren_o         => open,
        wren_ack_o     => open,
        rx_bit_reg_o   => open,
        state_dbg_o    => open
        );


		  
process (CLK, spi_do_valid, spi_do)
begin
	if (rising_edge(CLK)) then
		if spi_do_valid = '1' then
			case spi_do(15 downto 8) is 
				-- keyboard matrix
				when X"01" => kb_data(7 downto 0) <= spi_do (7 downto 0);
				when X"02" => kb_data(15 downto 8) <= spi_do (7 downto 0);
				when X"03" => kb_data(23 downto 16) <= spi_do (7 downto 0);
				when X"04" => kb_data(31 downto 24) <= spi_do (7 downto 0);
				when X"05" => kb_data(39 downto 32) <= spi_do (7 downto 0);	
				when X"06" => reset <= spi_do(0); turbo <= spi_do(1); magick <= spi_do(2); special <= spi_do(3);
				when others => null;
			end case;	
		end if;
	end if;
end process;

process (CLK)
begin
	if (rising_edge(CLK)) then 
		O_RESET <= not(reset);
		O_MAGICK <= not(magick);
		O_TURBO <= not(turbo);
		O_SPECIAL <= not(special);

		O_TEST <= reset;
		O_DEBUG <= kb_data(6); -- enter pressed

	end if;
end process;

O_CLK <= CLK;
		  
--    
process( kb_data, A)
begin
	if ( 
		(kb_data(0) = '1' and A(8) = '0') or 
		(kb_data(1) = '1' and A(9) = '0') or 
		(kb_data(2) = '1' and A(10) = '0') or 
		(kb_data(3) = '1' and A(11) = '0') or 
		(kb_data(4) = '1' and A(12) = '0') or 
		(kb_data(5) = '1' and A(13) = '0') or 
		(kb_data(6) = '1' and A(14) = '0') or 
		(kb_data(7) = '1' and A(15) = '0')
		) then 
		KB(0) <= '0';
	else
		KB(0) <= 'Z';
	end if;

	if ( 
		(kb_data(8) = '1' and A(8) = '0') or 
		(kb_data(9) = '1' and A(9) = '0') or 
		(kb_data(10) = '1' and A(10) = '0') or 
		(kb_data(11) = '1' and A(11) = '0') or 
		(kb_data(12) = '1' and A(12) = '0') or 
		(kb_data(13) = '1' and A(13) = '0') or 
		(kb_data(14) = '1' and A(14) = '0') or 
		(kb_data(15) = '1' and A(15) = '0')
		) then 
		KB(1) <= '0';
	else
		KB(1) <= 'Z';
	end if;	

	if ( 
		(kb_data(16) = '1' and A(8) = '0') or 
		(kb_data(17) = '1' and A(9) = '0') or 
		(kb_data(18) = '1' and A(10) = '0') or 
		(kb_data(19) = '1' and A(11) = '0') or 
		(kb_data(20) = '1' and A(12) = '0') or 
		(kb_data(21) = '1' and A(13) = '0') or 
		(kb_data(22) = '1' and A(14) = '0') or 
		(kb_data(23) = '1' and A(15) = '0')
		) then 
		KB(2) <= '0';
	else
		KB(2) <= 'Z';
	end if;	
	
	if ( 
		(kb_data(24) = '1' and A(8) = '0') or 
		(kb_data(25) = '1' and A(9) = '0') or 
		(kb_data(26) = '1' and A(10) = '0') or 
		(kb_data(27) = '1' and A(11) = '0') or 
		(kb_data(28) = '1' and A(12) = '0') or 
		(kb_data(29) = '1' and A(13) = '0') or 
		(kb_data(30) = '1' and A(14) = '0') or 
		(kb_data(31) = '1' and A(15) = '0')
		) then 
		KB(3) <= '0';
	else
		KB(3) <= 'Z';
	end if;	

	if ( 
		(kb_data(32) = '1' and A(8) = '0') or 
		(kb_data(33) = '1' and A(9) = '0') or 
		(kb_data(34) = '1' and A(10) = '0') or 
		(kb_data(35) = '1' and A(11) = '0') or 
		(kb_data(36) = '1' and A(12) = '0') or 
		(kb_data(37) = '1' and A(13) = '0') or 
		(kb_data(38) = '1' and A(14) = '0') or 
		(kb_data(39) = '1' and A(15) = '0')
		) then 
		KB(4) <= '0';
	else
		KB(4) <= 'Z';
	end if;	

end process;

end RTL;

