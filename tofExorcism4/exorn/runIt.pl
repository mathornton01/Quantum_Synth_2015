#!/usr/bin/perl

#my $found = `find /home/Kenneth/test/*.pla` ;
my $found = `find *.pla` ;

my @files = split(/\n/, $found) ;

my $alpha = 0.0 ;
my $beta = 1 - $alpha ;

while($alpha < 1.0) {

  #my $outDir = "/home/Kenneth/results/tfc.$alpha" ;
  my $outDir = "results/tfc.$alpha" ;
  my $mkdir = "mkdir -p $outDir" ;
  print "$mkdir\n" ;
  `$mkdir` ;

  foreach $file (@files) {
    my @tokens1 = split (/\//, $file) ;
    my $dir = "" ;
    for(my $i = 0 ; $i<$#tokens1 ; $i++) {
      $dir .= $tokens1[$i] . "/" ;
    }

    my $filename = $tokens1[$#tokens1] ;
    my @tokens2 = split(/\./, $filename) ;
    my $basename = $tokens2[0] ;
    print "==================================================\n" ;
    print "$dir $filename $basename $alpha $beta\n" ;
    #my $cmd = "./e.exe -n 1 -r 1 -a $alpha -b $beta -c 1 $file" ;
    my $cmd = "./e -n 1 -r 1 -a $alpha -b $beta -c 1 $file" ;
    print "\t$cmd\n" ;
    `$cmd` ;
    #move the outtputed esop and tfc files to the $outDir
    my $mvEsop = "mv $dir$basename.esop $outDir/." ;
    my $mvTfc = "mv $dir$basename.esop.tfc $outDir/." ;
    print "$mvEsop\n" ;
    `$mvEsop` ;
    print "$mvTfc\n" ;
    `$mvTfc` ;
  }

  $alpha += 0.1 ;
  $beta = 1 - $alpha ;
}
