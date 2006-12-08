#!/usr/bin/perl -w

# search for usable auto* toolset

@aclocal_versions = ("1.9", "1.8", "1.7");
@automake_versions = ("1.9", "1.8", "1.7");

@autoconf_versions = ("2.61", "2.60a", "2.59", "2.57", "2.5");


$use_aclocal = 0;
$use_automake = 0;
$use_automake = 0;

sub testversion {
  $version = shift;

  for $v (@_) {
    if ($version =~ /$v/) {
      return 1;
    }
  }
  return 0;
}

$paths = $ENV{'PATH'};
@paths = split(":", $paths);

@aclocal = ();
@automake = ();
@autoconf = ();

for $p (@paths) {
  @aclocal = (@aclocal, glob("$p/aclocal*"));
  @automake = (@automake, glob("$p/automake*"));
  @autoconf = (@autoconf, glob("$p/autoconf*"));
}

print "Testing aclocal:\n";
for $a (@aclocal) {
  open (VERSION, "$a --version |") || next;
  $v = <VERSION>;
  close (VERSION);
  $v =~ /.* (\S+)/;
  $usable = testversion($1, @aclocal_versions);
  print "  $a " . ("[OLD]", "[OK]")[$usable] . "\n";
  $use_aclocal = $a if (!$use_aclocal && $usable);
}

print "Testing automake:\n";
for $a (@automake) {
  open (VERSION, "$a --version |") || next;
  $v = <VERSION>;
  close (VERSION);
  $v =~ /.* (\S+)/;
  $usable = testversion($1, @automake_versions);
  print "  $a " . ("[OLD]", "[OK]")[$usable] . "\n";
  $use_automake = $a if (!$use_automake && $usable);
}

print "Testing autoconf:\n";
for $a (@autoconf) {
  open (VERSION, "$a --version |") || next;
  $v = <VERSION>;
  close (VERSION);
  $v =~ /.* (\S+)/;
  $usable = testversion($1, @autoconf_versions);
  print " $a " . ("[OLD]", "[OK]")[$usable] . "\n";
  $use_autoconf = $a if (!$use_autoconf && $usable);
}

if ($use_aclocal && $use_automake && $use_autoconf) {

  print "Running aclocal ...\n";
  system($use_aclocal);

  print "Running automake ...\n";
  system($use_automake, qw( --foreign -a));

  print "Running autoconf ...\n";
  system($use_autoconf);

  print "Complete!\n";
} else {
  print "Error: Couln'd find required auto* tools!\n";
}
