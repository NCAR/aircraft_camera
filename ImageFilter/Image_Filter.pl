#!/usr/bin/perl
use strict;
print "\n";
#flags and defults
our $ext = "jpg";
our $speedhack = 0;
our $verbose = 0;
our $force = 0;
our $dir = "";
our $checkCont = 1;
our $checkDark = 1;
our $checkGnd = 1;
#Ajusting these values will change what images the script removes (higher values = more bright)
our @threshold;
our $listfiles = 1;
$threshold[0] = 50; #if Mean is low image is dark
$threshold[1] = 35; #if Standard Deviation is low image is mostly one color
our $project = -1;
our $flight = -1;
our $expectedDiff = 1;
our $speedstart = 25;
our $speedstop = 80;

if (@ARGV <= 0) {
	print<<EOF;
	NO COMMAND LINE ARGUMENTS ENTERED
	This script will search through an image directory and removes very dark images using imagemagick also removes images taken on the ground using flt_time script (Installed on Merlot).
	Usage:	./Image_Filter.pl [OPTIONS] TARGET_DIR

	Options:
		-ext:jpg	Sets file extention (Default: jpg)
		-maxmean:##	Sets maximum average pixel value (per color) (Default:50) {0-255}
		-maxstddev:##	Sets maximum pixel value standard deviation (per color) (Default:35) {0-255}
		-t:##		Time in seconds between image files (Default:1) {Used to check consistency}
		-proj:PLOWS	Sets project (Not needed if TARGET_DIR if of form */Raw_Data/<Project>/*/flight_number_<flightno>/* )
		-fltno:ff01	Sets flight number (Not needed if TARGET_DIR if of form */Raw_Data/<Project>/*/flight_number_<flightno>/* )
		-skipcont	Skips continuity check
		-skipdark	Skips dark image check/removal and removed only ground images (based on flt_time script)
		-skipgnd	Skips ground image check/removal
		-v		Verbose (prints file operations)
		-nolist		Does not display list of files to be removed before confirmation, does display # of files to be removed
		-f		Force (Skips confimation and does not display list of dark images)
		-r		PERMANATLY REMOVE (normally dark images are moded to target_DIR/removed/) [NOT YET IMPLEMENTED]
		-tdir:removed	Sets name of subdir that dark images are moved to (Default: removed) [NOT YET IMPLEMENTED]
		-s:##		File skipping, Checks every ## files and assumes the images inbetween based on the images checked (Default: 0) {0-inf}
		-s%:##		Sets file skipping to a percentage of total files {0-100} [NOT YET IMPLEMENTED]
EOF
	die "";	
}
else {
	#Process command line arguments
	foreach my $a (@ARGV) {
		if ($a =~ m/-ext:(\S*)/) {$ext = $1;}
		elsif ($a =~ m/-maxmean:(\d+)/) { $threshold[0] = $1; }
		elsif ($a =~ m/-maxstddev:(\d+)/) { $threshold[1] = $1; }
		elsif ($a =~ m/-t:(\d+)/) { $expectedDiff = $1; }
		elsif ($a =~ m/-proj:(.*)/) { $project = $1; }
		elsif ($a =~ m/-fltno:(.*)/) { $flight = $1; }
		elsif ($a eq "-skipcont") { $checkCont = 0;}
		elsif ($a eq "-skipdark") { $checkDark = 0;} 
		elsif ($a eq "-skipgnd") { $checkGnd = 0;}
		elsif ($a eq "-v") {$verbose = 1;}	
		elsif ($a eq "-nolist") { $listfiles = 0; }
		elsif ($a eq "-f") {$force = 1;}
		elsif ($a eq "-r") { } #NOT IMPLEMENTED 
		elsif ($a =~ m/-tdir:(.*)/) {} #NOT IMPLEMENTED
		elsif ($a =~ m/-s:(\d+)/) {$speedhack = $1; }
		elsif ($a =~ m/-s%:(\d+)/) {} #NOT IMPLEMENTED
		elsif (-d $a) {$dir = $a;} #target dir
		elsif ($a eq "" || $a eq "\n") {} #For capturing stangeness
		else {die "INVALID ARGUMENT: $a";}	
	}
	if ($dir eq "") {die "NO DIRECTORY ENTERED: $dir";}
}

print "Reading directory: $dir\n";
opendir(DIR, "$dir");
our @files = readdir(DIR);
close(DIR);
print "Removing invalid files\n";
#ignore files that don end in .$ext
#for (my $i = 0; $i < $#files+1; $i++)
#{
	#if ($files[$i] !~ m/.*\.$ext/) { delete($files[$i]); }
	@files = grep(/.*\.$ext/, @files);
#}
#Sort files, this allows the script to skip files and accuratly assume what the files skipped contain
if ($speedhack || $checkGnd || $checkCont) { print "Sorting Files\n"; @files = sort { lc($a) cmp lc($b) } @files; }

#Variable declarations
my @filesToRemove;

if ($checkCont) {
	my $diff = 0;
	for (my $i = 1; $i <= $#files; $i++) {
		if ($i % int(($#files+1)/20)  == 0) { print "\rScanning for discontinuity: " . int($i/($#files+1)*100) . "% Done"; }	
		if ($files[$i] ne "" && $files[$i-1] ne "") {
			$diff = DateDiff(DateFromFile($files[$i]), DateFromFile($files[$i-1]));
			if (($diff > $expectedDiff && $diff > 0) || $diff < 0) {
				print "\nDate discontinuity between $files[$i] and $files[$i-1] | $diff seconds\n";
				print "To ignore press enter, otherwise input anyvalue:";
				my $cont = <STDIN>;
				if ($cont ne "\n") { die "Inconsistent data!\n"; }
			}
			elsif ($diff == 0) {
				print "\nUnable to detect time difference between:\n     $files[$i]\n     $files[$i-1]\n";
				print "To ignore press enter, otherwise input anyvalue:";
				my $cont = <STDIN>;
				if ($cont ne "\n") { die "Inconsistent data!\n"; }
			}
		}
	}
	print "\rScanning for discontinuity: 100% Done\n"; 
}
else
{
	print "\rScanning for discontinuity: SKIPPED\n"; 
}

our @date;
#date store in @date as MM,DD,YYYY,hh,mm,ss
if ($checkGnd) {
	#variable declaration for Gnd checking
	#my @date;
	my $speed = $speedstart;
	my $count = 0;
	#if project was not given try to get it out of the directory patch
	if ($project == -1) {
		$dir =~ m/\/Raw_Data\/(.*?)\//;
		$project = $1;
		if ($project eq "") { print "Unable to find project name, Gnd check may be skipped\n";}
	}
	#if flight number was not given try to get it out of the directory path
	if ($flight == -1) {
		$dir =~ m/(?:f|F)light_number_(\S\S\d\d)/;
		$flight = $1;
		if ($flight eq "") { print "Unable to find flight number, Gnd check may be skipped\n"; }
	}
	#location of netCDF file
	my $netCDF = "/scr/raf/Prod_Data/$project/$project$flight.nc";
	
	if (-e $netCDF) { #if the netCDF file is found
		#run flt_time scrpt and pipe the output into perl
		while ($#date+1 < 2 && $speed <= $speedstop) { 
			@date = ();
			open(DATA, "flt_time -t $speed $netCDF|");	
			#get takeoff and landing time from flt_time output
			$count = 0;
			while(<DATA>) {
				#while flt_time is outputting
				if ($_ =~ m/\s*\w\w\w (\w\w\w)\s*(\d?\d) (\d\d)\:(\d\d)\:(\d\d) (\d\d\d\d)\s*/) {
					$date[$count][0] = MonthToNum($1); #month
					$date[$count][1] = $2;#day
					$date[$count][3] = $3;#hour
					$date[$count][4] = $4;#min
				$date[$count][5] = $5;#sec
				$date[$count][2] = $6;#year
				if ($date[$count][2] < 100) { $date[$count][2] += 2000; }
					$count++;
				}
			}
			close(DATA);
			$speed++;
		}
		
		if ($#date+1 == 2) { 
			#unfortionatly flt_time only outputs one date/time and does not specify if it is a start or end time
			#so we have to check it if only put out 2, a start and stop
			#also this will filter out multiple flights in once netcdf file
			
			#print out takeoff | first file date and landing | last file date
			print "Takeoff: "; PrintDate(@{$date[0]}); print " | "; PrintDate(DateFromFile($files[0])); print "\n";
			print "Landing: "; PrintDate(@{$date[1]}); print " | "; PrintDate(DateFromFile($files[$#files])); print "\n";
			
			#Quick sanity check
			if (IsOlder(DateFromFile($files[$#files]), DateFromFile($files[0]))) {
				#earliest file should be earler than landing time
				if (IsOlder(DateFromFile($files[0]),@{$date[1]})) { 
					print "Earlist file is older than landing time! (";
					PrintDate(DateFromFile($files[0])); print " | ";
					PrintDate(@{$date[1]}); print ")\n";
					print "Scanning for ground images: SKIPPED (Bad Takeoff/LandingTime)\n"; 
					$checkGnd = 0;
				}
				#oldest file should be older than takeoff time
				if (IsOlder(@{$date[0]},DateFromFile($files[$#files]))) {
					print "Oldest file is earlier than takeoff time! (";
					PrintDate(@{$date[0]}); print " | ";
					PrintDate(DateFromFile($files[$#files])); print ")\n";
					print "Scanning for ground images: SKIPPED (Bad Takeoff/LandingTime)\n"; 
					$checkGnd = 0;
				}
			}
			else
			{
				print "Files sorted into reverse order?!\n";
			}
			if ($checkGnd) {
				#now check each file to see if it is newer than takeoff or older than landing
				for (my $i = 0; $i <= $#files; $i++) {
					if ($i+1 % int(($#files+1)/20)  == 0) { print "\rScanning for ground images: " . int($i+1/($#files+1)*100 + .5) . "% Done"; }
					#if file time is older than landing time remove it
					if (IsOlder(DateFromFile($files[$i]),@{$date[1]})) { push(@filesToRemove, $i);} 
					#if file time is earlier than takeoff time remove it
					if (IsOlder(@{$date[0]},DateFromFile($files[$i]))) { push(@filesToRemove, $i);}
				}
				print "\rScanning for ground images: 100% Done";
				
				if ($#filesToRemove+1 > 0) {print " (Ground Images Found!)\n"; Remove(@filesToRemove); @filesToRemove = ();}
				else { print " (No ground Images found)\n"; }
			}
		}
		else
		{
			print "\rScanning for ground images: SKIPPED (Missing takeoff or landing time)\n"; 
		}
	}	
	else {
		print "Could not find netCDF file, skipping ground image removal.\n";
		print "\rScanning for ground images: SKIPPED\n"; 	
	}
}
else { print "\rScanning for ground images: SKIPPED\n"; } 

if ($checkDark) {
	#dark checking variable declarations
	my $speeddark = 0; #this will be set to 1 if a dark file is encountered
	my @speedHist; #list of files skipped in the last iteration
	my @filesToRemove; #list of files idetified as dark

	print "Scanning for dark images: 0% Done";
	for (my $i = 0; $i <= $#files; $i++) {
		if ($i+1 % int(($#files+1)/20)  == 0) { print "\rScanning for dark images: " . int($i+1/($#files+1)*100 + .5) . "% Done"; }
		#skip files with the wrong ext Shoulden't be nessesary but just in case
		if ($files[$i] =~ m/.*\.$ext/) {
			#always true if $speedhack is 0, other wise skips $speedhack # of files and checks the very last file
			if ($speedhack == 0 || $i % $speedhack == 0 || $i == $#files) {
				#if image is dark store its name and deal with it later
				if (IsDark($files[$i])) {
					#$file is dark
					push (@filesToRemove, $files[$i]);
					$speeddark = 1;
					if ($speeddark && $speedhack) {
						#if image was dark and last image was dark all images inbetween should be dark
						foreach my $f (@speedHist) { push(@filesToRemove, $f); }
					}
					elsif ($speedhack) {
						#if speed hack is enabled skipped files need to be checked
						foreach my $f (@speedHist) {
							if (IsDark($files[$f])) { push(@filesToRemove, $f); }
						}
					}
				}
				elsif ($speeddark && $speedhack) {
					#if image was bright and last image was dark
					#check skipped images
					foreach my $f (@speedHist) {
						if (IsDark($files[$f])) { push(@filesToRemove, $f); }
					}
					$speeddark = 0; #this image was light
				}
				#clear file history
				@speedHist = ();
			}
			else {
				#add file to list of files skipped during this iteration
				push(@speedHist, $i);
			}
		}
	}
	print "\rScanning for dark images: 100% done\n";
	if ($#filesToRemove+1 > 0) {
		print "Dark images found!\n";
		Remove(@filesToRemove);
		@filesToRemove = ();
	}
	else {print "No Dark images found!\n";}
}
else { print "\rScanning for dark images: SKIPPED\n"; }



####
##Functions
####

sub PrintDate
{
	print "$_[0]/$_[1]/$_[2] $_[3]$_[4]$_[5]";
}

sub DateFromFile
{
	if ($_[0] =~ m/(\d\d)(\d\d)(\d\d)[\_|-](\d\d)(\d\d)(\d\d)\.$ext/) {
		my @date;
		$date[0] = $2; #Month
		$date[1] = $3; #Day
		$date[2] = $1; #Year
		if ($date[2] < 100) { $date[2] += 2000; }
		$date[3] = $4; #hour
		$date[4] = $5; #min
		$date[5] = $6; #sec
		#print "$date[0]/$date[1]/$date[2] $date[3]$date[4]$date[5]\n";
		return @date;
	}
}

sub DateDiff
{
	my @date1 = @_[0..5];
	my @date2 = @_[6..11];
	my $diff = 0; #in seconds
	
	#print "$date1[0]/$date1[1]/$date1[2] $date1[3]$date1[4]$date1[5]  |  $date2[0]/$date2[1]/$date2[2] $date2[3]$date2[4]$date2[5]\n";
	
	#if ($date1[0] != $date2[0]) {print "Date difference in months!\n"; return 0; }
	if ($date1[2] != $date2[2]) {print "\nDate difference in years!\n"; return 0; }
	
	$diff += ($date1[1] - $date2[1])*86400; #days
	$diff += ($date1[3] - $date2[3])*3600; #hours
	$diff += ($date1[4] - $date2[4])*60; #mins
	$diff += ($date1[5] - $date2[5]); #seconds
	return $diff;
}

sub MonthToNum
{
	my @months = ("Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Oct", "Nov", "Dec");
	for (my $i = 1; $i <= 12; $i++)
	{
		if ($_[0] eq $months[$i-1]) { return $i; }
	}
	die "Invalid month: $_[0]\n";
}

sub IsOlder
{
	#checks if the first inputted date is older than the second inputted date
	my @date1 = @_[0..5];
	my @date2 = @_[6..11];
	#expand year if need be
	if ($date1[2] < 100) { $date1[2] += 2000; }
	if ($date2[2] < 100) { $date2[2] += 2000; }
	#if year is larger its older
	if ($date1[2] > $date2[2]) { return 1; }
	if ($date1[2] < $date2[2]) { return 0; }
	#if month is larger its older
	if ($date1[0] > $date2[0]) { return 1; }
	if ($date1[0] < $date2[0]) { return 0; }
	#if day is larger its older
	if ($date1[1] > $date2[1]) { return 1; }
	if ($date1[1] < $date2[1]) { return 0; }
	#if date year month is equal then check hour
	if ($date1[3] > $date2[3]) { return 1; }
	if ($date1[3] < $date2[3]) { return 0; }
	#no? houabout minute difference
	if ($date1[4] > $date2[4]) { return 1; }
	if ($date1[4] < $date2[4]) { return 0; }
	#Wow these dates are alost exactly the same!
	#how about seconds?
	if ($date1[5] > $date2[5]) { return 1; }
	if ($date1[5] < $date2[5]) { return 0; }	
	#these date are the same
	return -1;
}
sub Remove
{
	my $cont = "\n";
	if ($force == 0) {
		#print a list of files to be removed
		if ($listfiles) {
			foreach my $i (@_) {
				print "Remove: $files[$i]\n";
			}
		}
		print "Total files to remove: " . ($#_+1) . " of " . ($#files+1) . "\n";
		
		print "Takeoff: "; PrintDate(@{$date[0]}); print " | "; PrintDate(DateFromFile($files[0])); print "\n";
		print "Landing: "; PrintDate(@{$date[1]}); print " | "; PrintDate(DateFromFile($files[$#files])); print "\n";

		print "To continue press enter otherwise enter any value:";
		#get user input (this will capture trailing \n)
		$cont = <STDIN>;
	}

	if ($cont eq "\n") {
		print "\nCreating temporary directory..\n";
		mkdir("$dir/removed/") or die $!;	
	
		#remove all files identified as dark
		foreach my $i (@_) {
			if ($verbose) {print "Moving: $dir/$files[$i] To $dir/removed/$files[$i]\n";}
			rename("$dir/$files[$i]","$dir/removed/$files[$i]") or die $!; #rename == move
		}
		#remove removed files from the files array
		foreach my $i (@_) { splice(@files, $i, 1); }
	}
}

sub IsDark
{
	my @colordata;
	$colordata[0][0] = -1;
	$colordata[0][1] = -1;
	my $colorindex = -1;
	my $file = @_[0];
	#use imagemagick to get picture information, piped into DATA
	open(DATA, "identify -verbose  \"$dir/$file\"|");
	#Extract mean and skewness from output 
	while (<DATA>) {
		#use regexpressions to get data. In newer versions of imagemagick there is no capitalization..
		#(?: ) is a non capturing group vs. ( ) which captures and is stored in $1 $2 etc.
		#\s* is any amount of whitespace, used lberally to prevent updates from breaking the script
		if ($_ =~ m/\s*(?:R|r)ed:\s*/) {
			$colorindex = 0;
		}
		elsif ($_ =~ m/\s*(?:G|g)reen:\s*/) {
			$colorindex = 1;
		}
		elsif ($_ =~ m/\s*(?:B|b)lue:\s*/) {
			$colorindex = 2
		}
		elsif ($_ =~ m/\s*(?:M|m)ean:\s*(\d*\.?\d*)\s*(\s*\(\d*\.\d*\))?\s*/) {
			$colordata[$colorindex][0] = $1;
		}
		elsif ($_ =~ m/\s*(?:S|s)tandard (?:D|d)eviation:\s*(\d*\.?\d*)\s*(\s*\(\d*\.\d*\))?\s*/) {
			$colordata[$colorindex][1] = $1;
		}
	}	
	close DATA;
	
	if ($colordata[0][0] == -1 || $colordata[0][1] == -1 || $colorindex == -1) {
		die "\nImageMagick Parsing Failure: $file\n$colordata[0][0]|$colordata[0][1]|$colorindex";
	}

	my $dark = 1; #dark unless otherwise proven by jury trial
	for (my $i = 0; $i < 3; $i++) {
		#print "$colordata[$i][0] | $colordata[$i][1]\n";
		#if mean or standard deviation is above threshold image is not dark
		if ($colordata[$i][0] > $threshold[0]) {
			$dark = 0;
			last;
		}
		elsif ($colordata[$i][1] > $threshold[1]) {
			$dark = 0;
			last;
		}  
	}
	#if ($dark) { print "DARK \n"; }
	return $dark;

}
