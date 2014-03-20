#!/usr/bin/perl

# lirc-udp.pl -  A simple utility for sending mode2 LIRC codes to a UDP socket.
# Version 1.0 - February 17th, 2008
# Developed by Jason Pimble.  Copyright 2008, licensed as public domain.

# This expects LIRC to be listening on port 5000 for udp mode2 packets.  This
# is the normal case if you start lircd like this:

# /usr/sbin/lircd --driver=udp --device=5000
#
# If you want to be able to use your IR/USB remote too, you need to network
# multiple lircd instances together:

# /usr/sbin/lircd --driver=default --device=/dev/lirc0 --listen
# /usr/sbin/lircd --driver=udp --device=5000 --pidfile=/var/run/lirc2.pid --connect=localhost:8765

# Only tested with a SPACE_ENC|CONST_LENGTH remote definition (pioneer CU-VSX107).  

# If using with a non-mode2 remote, just take the mode2 header/one/zero/ptrail/gap numbers from a mode2 remote, and 
# put them below, and in a new remote definition in your lircd.conf

# The following values come from your lircd.conf:  

#  header       8497  4305
#  one           515  1618
#  zero          515   550
#  ptrail        515
#  gap          90010

use strict;

my $header_pulse = 8497;
my $header_space = 4305;
my $one_pulse = 515;
my $one_space = 1618;
my $zero_pulse = 515;
my $zero_space = 550;
my $trail_pulse = 515;
my $gap_space = 90010;

# Button definitions come from your lircd.conf too:

my %buttons = (
          Home                     => "0x14D5000000",
          Playlist                 => "0x14D6010000",
          Album                    => "0x14EE190000",
          Artist                   => "0x14F01B0000",
          Genre                    => "0x14F6210000",
          Track                    => "0x14F8230000",
          LiveTV                   => "0x14D8030000",
          DVD                      => "0x14D9040000",
          WEB                      => "0x14DA050000",
          Guide                    => "0x14DB060000",
          TV                       => "0x14DC070000",
          Power                    => "0x14D7020000",
          MOUSE_LEFT_BTN           => "0x144D780000",
          MOUSE_RIGHT_BTN          => "0x14517C0000",
          MOUSE_UP                 => "0x1447720000",
          MOUSE_DOWN               => "0x1448730000",
          MOUSE_LEFT               => "0x144C770000",
          MOUSE_RIGHT              => "0x1446710000",
          VolDown                  => "0x14DD080000",
          VolUp                    => "0x14DE090000",
          Mute                     => "0x14DF0A0000",
          ChanUp                   => "0x14E00B0000",
          ChanDown                 => "0x14E10C0000",
          1                        => "0x14E20D0000",
          2                        => "0x14E30E0000",
          3                        => "0x14E40F0000",
          4                        => "0x14E5100000",
          5                        => "0x14E6110000",
          6                        => "0x14E7120000",
          7                        => "0x14E8130000",
          8                        => "0x14E9140000",
          9                        => "0x14EA150000",
          0                        => "0x14EC170000",
          Add_Delete               => "0x14EB160000",
          A_B                      => "0x14ED180000",
          Up                       => "0x14EF1A0000",
          Down                     => "0x14F7220000",
          Left                     => "0x14F21D0000",
          Right                    => "0x14F41F0000",
          Enter                    => "0x14F31E0000",
          PageUp                   => "0x14F11C0000",
          PageDown                 => "0x14F5200000",
          Rewind                   => "0x14F9240000",
          Play                     => "0x14FA250000",
          Forward                  => "0x14FB260000",
          Record                   => "0x14FC270000",
          Stop                     => "0x14FD280000",
          Pause                    => "0x14FE290000",
);

use IO::Socket::INET;

sub get_high_byte {
  my $microseconds = shift;
  
#  my $value = ($microseconds * 16384) / 1000000;
  my $value = ($microseconds * 256) / 15625;

  $value = $value / 256;

  return $value;
}

sub get_low_byte {
  my $microseconds = shift;
  
#  my $value = ($microseconds * 16384) / 1000000;
  my $value = ($microseconds * 256) / 15625;

  $value = $value % 256;

  return $value;
}

sub encode_msg {
  my $in_msg = shift;
  my $out_msg = "";

  use bytes;

  my $zero_pulse1 = get_low_byte($zero_pulse);
  my $zero_pulse2 = get_high_byte($zero_pulse);
  my $zero_space1 = get_low_byte($zero_space);
  my $zero_space2 = get_high_byte($zero_space) | 0x080;
  my $one_pulse1 = get_low_byte($one_pulse);
  my $one_pulse2 = get_high_byte($one_pulse);
  my $one_space1 = get_low_byte($one_space);
  my $one_space2 = get_high_byte($one_space) | 0x080;

  my $header_pulse1 = get_low_byte($header_pulse);
  my $header_pulse2 = get_high_byte($header_pulse);
  my $header_space1 = get_low_byte($header_space);
  my $header_space2 = get_high_byte($header_space) | 0x080;

  my $trail_pulse1 = get_low_byte($trail_pulse);
  my $trail_pulse2 = get_high_byte($trail_pulse);

  my $gap_space1 = get_low_byte($gap_space);
  my $gap_space2 = get_high_byte($gap_space) | 0x080;

# first we need a gap

  $out_msg = $out_msg . chr($gap_space1) . chr($gap_space2);

# then we need the header

  $out_msg = $out_msg . chr($header_pulse1) . chr($header_pulse2) . chr($header_space1) . chr($header_space2); 

# then all the bits in the code

  for (my $i=0;$i<length($in_msg);++$i) {
    my $byte = ord(substr($in_msg,$i,1));

    for ($b=7;$b>=0;--$b) {
      my $mask = 1 << $b;

      if(($byte & $mask) == 0) {
        $out_msg = $out_msg . chr($zero_pulse1) . chr($zero_pulse2) . chr($zero_space1) . chr($zero_space2);
      }
      else {
        $out_msg = $out_msg . chr($one_pulse1) . chr($one_pulse2) . chr($one_space1) . chr($one_space2);
      }
    }
  } 

  $out_msg = $out_msg .chr($trail_pulse1) . chr($trail_pulse2);

  no bytes;

  return $out_msg;
}

sub print_as_hex {
  my $msg = shift;
  my $i;

  use bytes;

  for ($i=0;$i<length($msg);++$i) {
    my $byte = ord(substr($msg,$i,1));
    printf "%02x ", $byte;
  }
  print "\n";

  no bytes;
}

sub hex_string_to_bytes_string {
  my $hex_string = shift;
  my $byte_string;

  my $i;

# strip off 0x if it exists

  if(substr($hex_string,0,2) eq "0x") {
    $hex_string = substr($hex_string,2);
  }

# strip off any pairs of 0s in the front

  while(substr($hex_string,0,2) eq "00") {
    $hex_string = substr($hex_string,2);
  }

# convert the remaining hex string to bytes

  for ($i=0;$i<length($hex_string);$i+=2) {
    my $byte_val = hex(substr($hex_string,$i,2));
    use bytes;
    $byte_string = $byte_string . chr($byte_val);
    no bytes;
  }

# return the hex string

  return $byte_string;
}

# Send a mode2 code to the UDP broadcast port.

sub send_code {
  my $byte_string = shift;

# bind a network port for broadcasting the packet

  my $MySocket = new IO::Socket::INET->new(PeerPort=>5000,
                                      Proto=>'udp',
                                      PeerAddr=>inet_ntoa(INADDR_BROADCAST),Broadcast=>1) or die "Can't bind : $@\n";

  my $enc_msg = encode_msg($byte_string);
#  print_as_hex($enc_msg);

  # Send message

  $MySocket->send($enc_msg);

  $MySocket->close();
}

# Send a named button as mode2 codes to a UDP lirc

sub send_button {
  my $button_name = shift;

  # Get the byte-string version of the button code

  my $button_byte_code = hex_string_to_bytes_string($buttons{$button_name});

  # send the button code via UDP

  send_code($button_byte_code);
}

##############################################################################
##############################################################################
####                                                                      ####
####                           Main Entry Point                           ####
####                                                                      ####
##############################################################################
##############################################################################

# take each argument, 

for (my $argnum=0;$argnum<=$#ARGV;$argnum++) {
  send_button($ARGV[$argnum]);
}