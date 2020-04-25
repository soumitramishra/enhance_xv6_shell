#!/usr/bin/perl
use 5.16.0;
use warnings FATAL => 'all';

use Test::Simple tests => 16;

use Expect;

# If you want to see stdout, during
# tests for debugging, you can comment this out.
$Expect::Log_Stdout = 0;

my $TO = 10;
my ($good,$good1,$good2,$good3,$good4,$text);

system(qq{make});

my $tty = Expect->spawn(qq{timeout -k 60 55 make qemu});
$tty->expect($TO, "init: starting sh");
sleep(1);
# No aliases
$tty->send("alias\n");
$good = $tty->expect($TO, "Total aliases: 0");
ok($good, "No aliases shows 0 total aliases");

# Alias not found-Zero Aliases
$tty->send("alias abc\n");
$good = $tty->expect($TO, "alias: abc: not found");
ok($good, "Invalid alias:Zero aliases : alias not found message");

# Add one alias
$tty->send("alias l=ls\n");
$good = $tty->expect($TO, "alias::Added alias l");

# Check if alias is added or not
$tty->send("alias\n");
$good = $tty->expect($TO, "Total aliases: 1");
ok($good, "Add one alias");

# Check if alias is printed or not
$good = $tty->expect($TO, "alias l = 'ls'");
ok($good, "Print alias");

# Add second alias
$tty->send("alias t1=sh test1.sh ; done sh1\n");
$good = $tty->expect($TO, "alias::Added alias t1");

# Check if alias is added or not
$tty->send("alias\n");
$good = $tty->expect($TO, "Total aliases: 2");
ok($good, "Add second alias");

# Check if alias is printed or not
$good1 = $tty->expect($TO, "alias l = 'ls'");
$good2 = $tty->expect($TO, "alias t1 = 'sh test1.sh ; done sh1'");
$good = $good1 && $good2;
ok($good, "Print all aliases after adding two aliases");

# Add third alias with leading white space after name and trailing whitespace before value
$tty->send("alias t2  =   sh test2.sh ; done sh2\n");
$good = $tty->expect($TO, "alias::Added alias t2");

# Check if alias is added or not
$tty->send("alias\n");
$good = $tty->expect($TO, "Total aliases: 3");
ok($good, "alias::Added alias leading white space after name and trailing whitespace before value");

# Check if alias is printed or not
$good1 = $tty->expect($TO, "alias l = 'ls'");
$good2 = $tty->expect($TO, "alias t1 = 'sh test1.sh ; done sh1");
$good3 = $tty->expect($TO, "alias t2 = 'sh test2.sh ; done sh2'");
$good = $good1 && $good2 && $good3;
ok($good, "Print all aliases after adding three aliases");

# Print a particular alias
$tty->send("alias t1\n");
$good = $tty->expect($TO, "alias t1 = 'sh test1.sh ; done");
ok($good, "Print a particular alias");

# Alias not found-After Adding few Aliases
$tty->send("alias abc\n");
$good = $tty->expect($TO, "alias: abc: not found");
ok($good, "Invalid alias : after adding a few aliases : alias not found message");

# Running the scripts using alias
$tty->send("t1\n");
$tty->expect($TO, "test complete: sh1");
ok($tty->before() =~ /Hello, Script/, "Running alias t1");

# Update an alias
$tty->send("alias t1=status true; done true\n");
$good = $tty->expect($TO, "alias::Updated alias t1");
ok($good, "Alias updated successfully");

# Print the updated alias
$tty->send("alias t1\n");
$good = $tty->expect($TO, "alias t1 = 'status true; done true'");
ok($good, "Updated alias printed");

#Execute the updated alias
$tty->send("t1\n");
$tty->expect($TO, "test complete: true");
ok($tty->before() =~ /status = 0/m, "Running successfully updated alias t1 true: status = 0,");

#Print the help --help option
$tty->send("alias --help\n");
$good1 = $tty->expect($TO, "Usage:");
$good2 = $tty->expect($TO, "alias - to print all aliases");
$good3 = $tty->expect($TO, "alias alias_name - to print particular alias");
$good4 = $tty->expect($TO, "alias alias_name = cmd_name- to add/update alias");
$good = $good1 && $good2 && $good3 && $good4;
ok($good, "--help option");

#Print the help shortcut -h option
$tty->send("alias -h\n");
$good1 = $tty->expect($TO, "Usage:");
$good2 = $tty->expect($TO, "alias - to print all aliases");
$good3 = $tty->expect($TO, "alias alias_name - to print particular alias");
$good4 = $tty->expect($TO, "alias alias_name = cmd_name- to add/update alias");
$good = $good1 && $good2 && $good3 && $good4;
ok($good, "-h option");

$tty->send("halt\n");
sleep(1);
