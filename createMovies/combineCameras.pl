#!/usr/bin/perl -w

# Stuart Beaton, NCAR/RAF

# Generates an annotated MPEG-4 video from a directory of images and
# netcdf file. Reads a parameter file specified on the command line to
# get the netcdf file, image directory, variables, etc.

# 2006 Aug 23
# Added $cameraName and image adjustment keywords.
# Added default scale, outputResolution, and bit rate for axis camera.

# -------------------------------------------------------------------
# Files used:
#	parameters file specified on command line.
#	$netcdfFile - netCDF flight data file read from parameters file.

# names hardcoded in Initialization section:
#	$headerDump - netCDF header dump from ncdump.
#	$batchFile - n2asc batch input file.
#	$dataFile - flight values dumped from n2asc.

#	$annotatedImageDirectory 	= /tmp/AnnotatedImages for gni, 
#				= /scr/raf/beaton/AnnotatedImages for bora.
# -------------------------------------------------------------------

#Uses ...
use Image::Magick;		# Non-standard ImageMagick extensions.
use Sys::Hostname;		# standard module for determining hostname.

#if no arguments or argument is "-h", print help text and exit.
if ( (scalar(@ARGV) != 2) || ($ARGV[0] eq "-h") ) {
	print "\n\n Perl script to generate annotated movies from images\n";
	print "USAGE: movie.pl  parameterFileName flight<eg rf01>\n\n";
	exit;
	}

# Initialize image objects. 
$inputImage = Image::Magick->new();		# Forward image read in.
$downImage = Image::Magick->new();		# Downward image read in.
$outputImage = Image::Magick->new();	# Image to be written out.
$labelImage = Image::Magick->new();		# Image of variable labels.
$valueImage = Image::Magick->new();		# Image of variable values.

# -------------------------------------------------------------------
# ------------------------- Initialization --------------------------
# -------------------------------------------------------------------
# Get start time to create unique file names
$startTime = time();

# Hardcoded file names.
$headerDump="./janine_headerDump_$startTime";
$batchFile="./janine_batchFile_$startTime";
$dataFile="./janine_dataFile_$startTime.asc";
if (hostname() =~ m/gni/) {$annotatedImageDirectory = "/tmp/AnnotatedImages";}
if (hostname() =~ m/bora/) {$annotatedImageDirectory = "./AnnotatedImages";}

# Known cameras with default parameters available.
$knownCameras = "Axis,Forward_Flea, Down_Flea";

# Set defaults for variables
$annotationFont = "Courier-Bold";
$fontSize = "13";

# -------------------------------------------------------------------
# ------------------- Get annotation parameters ---------------------
# -------------------------------------------------------------------

# Read parameter file.
# Lines are in the form of 'parameter_name=value'.
# End parameters section with "endParameters".

# The name of the parameters file is the first command line argument.
-e $ARGV[0] or die "Parameters file doesn't exist\n";
open PARAMETERS_FILE, "<$ARGV[0]";

$flightNum = $ARGV[1];

# Loop through the parameters section of the file.
while (<PARAMETERS_FILE>)  {	# Read a line from the file
	s/\s//g;			# Remove all white space.
	chomp;				# Remove trailing newline.
	next if /^$/;			# Next line if this one is blank.
	next if /^#/;			# Next line if this one is commented out with #
	last if /^endParameters/ ;	# Exit loop if it contains this string.
	($keyword, $value) = split /=/;	# Split at equal sign.
	SWITCH: {	# A labelled bare block since Perl lacks switch/case statements.
		if ($keyword eq "netcdfFile") { $netcdfFile=$value;
		    $netcdfFile =~ s/####/$flightNum/; last SWITCH; }
		if ($keyword eq "cameraName") {$cameraName=$value; last SWITCH;}
		if ($keyword eq "imageDirectory") {$imageDirectory=$value;
		    $imageDirectory =~ s/####/$flightNum/; last SWITCH;}
		if ($keyword eq "outputResolution") { $outputResolution=$value; last SWITCH; }
		if ($keyword eq "outputFrameRate") { $outputFrameRate=$value; last SWITCH;}
		if ($keyword eq "scale") {$scale=$value; last SWITCH;}
		if ($keyword eq "gamma") {$gamma=$value; last SWITCH;}
		if ($keyword eq "sharpen") {$sharpen=$value; last SWITCH;}
		if ($keyword eq "annotationFont") {$annotationFont=$value; last SWITCH;}
		if ($keyword eq "fontSize") {$fontSize=$value; last SWITCH;}
		if ($keyword eq "mp4BitRate") { $mp4BitRate=$value; last SWITCH;}
		if ($keyword eq "crop") {$cropGeometry=$value; last SWITCH;}
		if ($keyword eq "includeProjectName") {$includeProjectName=$value; last SWITCH;}
		die "Quitting because of unrecognized keyword $keyword in file.\n";	# Fall-through.
	}
}

# -------------------------------------------------------------------
# Defaults for various cameras if not set in parameters file.
# -------------------------------------------------------------------
!$cameraName and die "Camera must be identified in the parameters file! \n";
$knownCameras !~ m/$cameraName/ and die "Unknown camera! \n";

if ($cameraName =~ m/Axis/) {	# rescale to square pixels (width / 1.1) in approximate widescreen (16:9) format and allowing room for annotation.
	if (!$scale) {$scale = "640x480!";}	# The '!' forces resize to exact dimensions, accounting for pixel aspect ratio.
	if (!$outputResolution) {$outputResolution = "800x480";}
	if (!$outputFrameRate) { $outputFrameRate = 15; }	# 15 fps playback when recorded at 1 fps.
	if (!$mp4BitRate) { $mp4BitRate = 1500000; }
}

if ($cameraName =~ m/Forward_Flea/) { 	# Reduce resolution from 1024x768, adjust gamma, and sharpen.
	if (!$scale) { $scale = "640x480"; }
	if (!$outputResolution) { $outputResolution = "800x480"; }
	if (!$sharpen) { $sharpen = "0.0x1.0"; }
	if (!$gamma) { $gamma = 1.1; }
	if (!$outputFrameRate) { $outputFrameRate = 15; }	# Playback at 15 fps when recorded at 1 fps.
	if (!$mp4BitRate) { $mp4BitRate = 1500000; }
}
if ($cameraName =~ m/Down_Flea/) { 	# Reduce resolution from 1024x768, adjust gamma, and sharpen.
	if (!$scale) { $scale = "640x480"; }
	if (!$outputResolution) { $outputResolution = "800x480"; }
	if (!$sharpen) { $sharpen = "0.0x1.0"; }
	if (!$gamma) { $gamma = 1.1; }
	if (!$outputFrameRate) { $outputFrameRate = 30; }	# Playback at 30 fps when recorded at 2 fps.
	if (!$mp4BitRate) { $mp4BitRate = 1500000; }
}


# -------------------------------------------------------------------
# ---------------- Get flight info from header file -----------------
# -------------------------------------------------------------------

# Create a new header dump.
-e $netcdfFile or die "Netcdf file $netcdfFile not found.\n";
if (system "ncdump -h $netcdfFile > $headerDump") {die "Couldn't create netcdf header dump file $headerDump"};

# Open the netCDF header dump file and get the project name, flight, and date:
open(HEADER_DUMP_FILE, "$headerDump") || die "Couldn't open header file $flightNumber.header\n";

if ($includeProjectName =~ m/yes/i) {	# match on yes, ignoring case.
	seek (HEADER_DUMP_FILE, 0,0);
	($projectName)=($tempVar[0] =~ /^\s+:ProjectName = "(.*)"/);
} else {
	$projectName='';		# Empty string.
}

seek(HEADER_DUMP_FILE,0,0);
@tempVar=grep(/:ProjectNumber =/, <HEADER_DUMP_FILE>);
($projectNumber)=($tempVar[0] =~ /^\s+:ProjectNumber = "(.*)"/);

seek(HEADER_DUMP_FILE,0,0);
@tempVar=grep(/:FlightNumber =/, <HEADER_DUMP_FILE>);
($flightNumber)=($tempVar[0] =~ /^\s+:FlightNumber = "(.*)"/);

seek(HEADER_DUMP_FILE,0,0);
@tempVar=grep(/:TimeInterval =/, <HEADER_DUMP_FILE>);
($timeInterval)=($tempVar[0] =~ /^\s+:TimeInterval = "(.*)"/);
($beginTime,$endTime) = split('-',$timeInterval);

seek(HEADER_DUMP_FILE,0,0);
@tempVar=grep(/:FlightDate =/, <HEADER_DUMP_FILE>);
($flightDate)=($tempVar[0] =~ /^\s+:FlightDate = "(.*)"/);
($mn,$dy,$yr) = split('/',$flightDate);
$time_interval = "$yr-$mn-$dy,$beginTime~$yr-$mn-$dy,$endTime";
print $time_interval."\n";

$prelim = "\n\n";
seek(HEADER_DUMP_FILE,0,0);
if (grep(/PRELIMINARY/, <HEADER_DUMP_FILE>)) {
	$prelim = "\nPRELIMINARY DATA \n";
}

# Close header dump file.
close (HEADER_DUMP_FILE);

$headerText="$projectName$projectNumber $flightNumber \n$flightDate \n $prelim ";

# -------------------------------------------------------------------
# Create n2asc input batch file, clobbering old one if it exists.
# This section copies the variable list from the parameters file
# into the n2asc batch file.
# -------------------------------------------------------------------
open BATCH_FILE, "> $batchFile"  or die "Can't open ascii batch file $batchFile\n";
print BATCH_FILE "if=$netcdfFile\nof=$dataFile\n";
#print BATCH_FILE "ti=2000-01-01,00:00:00~2008-12-31,23:59:59\n";
print BATCH_FILE "ti=$time_interval\n";


while (<PARAMETERS_FILE>) {
	s/\s//g;				# Remove all white space.
	chomp;					# Remove trailing newline.
	next if /^$/;				# Next line if it's blank.
	last if /endVariables/;		# Exit loop if it contains this string.
	print BATCH_FILE "var=$_\n";
}
close BATCH_FILE;
close PARAMETERS_FILE;	# Done with this file, but don't delete it.

# -------------------------------------------------------------------
# ------------------- Create & load data file -----------------------
# -------------------------------------------------------------------

# Remove old flight data file if it exists, create new one with N2ASC.
-e $dataFile and unlink $dataFile;
if (system "n2asc -b $batchFile") {die "Couldn't create flight data file $dataFile"};

# Open the data file created by the n2asc and read the heading.
open (FLIGHT_DATA_FILE, $dataFile) || die "\nCouldn't open the data file $dataFile\n";
@flightData=<FLIGHT_DATA_FILE>;				# Put the data file into an array variable.

# Get the header string from the first line of the data,
# then split this into variables to write to the image
@columnLabels=split(',',shift(@flightData));
$theText = '';
foreach $label (@columnLabels) { 
	$theText=$theText.sprintf("%s\n",$label);
}
#chomp $theText;		# Remove final newline.

# Create a small (unchanging) image consisting of the variable labels.
($outputWidth, $outputHeight) = split /x/, $outputResolution; 
@$labelImage = ();
$labelImage->SetAttribute(size=>"110x$outputHeight");
$labelImage->ReadImage('xc:none');		# Transparent canvas
$labelImage->Annotate( font=>$annotationFont, pointsize=>$fontSize, weight=>500, gravity=>'NorthWest', fill=>'black',text=>"$headerText");
$labelImage->Annotate( font=>$annotationFont, pointsize=>$fontSize, weight=>500, gravity=>'SouthWest', fill=>'black',text=>"$theText");

close (FLIGHT_DATA_FILE);		# Don't need the file any longer.

# -------------------------------------------------------------------
# ----------------------- Load image list ---------------------------
# -------------------------------------------------------------------

# Open the directory and read the list of JPEG files into @jpegFiles array.
opendir(IMAGE_DIRECTORY, "$imageDirectory") || die "Image directory $imageDirectory not found";
@tempList = grep{/.jpg/} readdir IMAGE_DIRECTORY;
closedir IMAGE_DIRECTORY;

# sort by filename which is the image time. 
# Alternative would be to search the entire list for each second of data.
# (Should see how much additional time that would add.)
@jpegFiles=sort @tempList;
$numFiles = scalar(@jpegFiles);
print "Number of images to process = $numFiles\n";

$annotatedImageDirectory = $annotatedImageDirectory."_$flightNumber";
# Delete old annotated images directory and create a new (empty) one.
# Little error checking here right now, and could use FILE::PATH functions.
-d $annotatedImageDirectory and system "rm -r $annotatedImageDirectory";
mkdir "$annotatedImageDirectory" or die "Couldn't create $annotatedImageDirectory directory!";
print "Annotated images will be stored in $annotatedImageDirectory\n";

# --------------------------------------------------------------------
#---------------------- Delete temporary files -----------------------
# --------------------------------------------------------------------

unlink $batchFile;
unlink $headerDump;
unlink $dataFile;

# --------------------------------------------------------------------
#---------------------- End of Initialization ------------------------
# --------------------------------------------------------------------

# --------------------------------------------------------------------
#---------------------- Image annotation loop ------------------------
# An alternative would be to get the time from the data file and load
# the appropriate image. If that image is missing, just keep going.
# --------------------------------------------------------------------

# Find a matching time in the data file
$fileNum=0;
foreach $fileName (@jpegFiles) {	
	$fileNum++;		#track the number of image files processed

	# clear the images, set output attributes, then read and scale an image
	@$outputImage = ();
	@$inputImage = ();
	@$valueImage = ();
	@$downImage = ();
	
	# Size output image with white background (canvas).
	$outputImage->Set(size=>$outputResolution, quality=>90);
	$outputImage->ReadImage('xc:white');	# White canvas
	
#	This is for the forward image.
	$inputImage->ReadImage("$imageDirectory/$fileName");
	# Apply image adjustments. Should sharpening be before or after scaling? (After is quicker, is before better?)
	if ($cropGeometry) {$inputImage->Crop( geometry=>$cropGeometry )};
	if ($scale) {$inputImage->Scale( geometry=>$scale )};
	if ($gamma) {$inputImage->Gamma( gamma=>$gamma );}
	if ($sharpen) {$inputImage->Sharpen( geometry=>$sharpen )};	# radius = 0, sigma = 1 is good.
	
	# Initialize image used for variable values.
	$valueImage->Set( size=>$outputResolution );
	$valueImage->ReadImage('xc:none');		# Transparent canvas

	# The last 6 characters of the filename before the .jpg is the time of the camera or filesaving-computer.
	# I'll ignore the date which preceeds the time since the ascii file doesn't include it.
	# Since the date's in the file name flights that roll over to UTC 000000 are still sorted properly.
	$imageTime=substr($fileName,-10,6);
	$imageTime_withColons=substr($imageTime,0,2).':'.substr($imageTime,2,2).':'.substr($imageTime,4,2);
	print "image $fileNum/$numFiles: $fileName\n";

#	For down image, FLEA camera.
#	Change directory path ending from _fwd to _down
	$downDirectory = sprintf("%s%s",(substr $imageDirectory,  0, (length $imageDirectory)-4),"_down");

#	Get last part of file name, minus .jpg extension
	$downName=substr $fileName,-17,13;
#	Swap '-' for '_' in yymmdd-hhmmss. This is fixed in later projects.
	$downName =~ s/_/-/;
#	Add .jpg extension
#	$downName=sprintf "%s_0.jpg",$downName;
	$downName=sprintf "%s.jpg",$downName;
printf "%s\n",$downName;
	$downImage->ReadImage("$downDirectory/$downName") or $downImage->ReadImage('xc:white');
	printf "%s\n", "$downDirectory/$downName";
	$downImage->Sharpen(geometry=>'0.0x1.0');
	$downImage->Scale(geometry=>$scale );

	# Get the data from the data file and put into an array. If the the datafile time does not
	# match the image filename time, get the next line of the data file until it does.
	# This assumes there are no time gaps in the ascii file generated from the netCDF file.
	# (This is a safe assumption, especially with production data. BUT the first image must
	# be equal to or later than the first data point.)
	while (substr($flightData[0],11,8) ne $imageTime_withColons)  {
#	print "M";	# Can count 'M's to find out how many images were missing.
	shift(@flightData);
		if (scalar(@flightData) == 0) {
			die "End of data file reached searching for $fileName";
			exit;	# If data is missing, the program exits. Should have this continue to next image.
		}
	}
	
	# Create the data string.
	@dataItems = split(',',shift(@flightData));
	$theText = '';
	foreach $value (@dataItems) { 
		$theText=$theText.sprintf("%s\n", $value);
	}
#	chomp $theText;		# Remove final newline.
	
	# Create an image consisting of the variable values.
	$valueImage->Annotate( font=>$annotationFont, pointsize=>$fontSize, weight=>500, gravity=>'SouthEast', fill=>'black',text=>"$theText");
	
	# Composite all three images (variable values, labels, and camera image).
	# Put the values along the lower right side of the image
	$outputImage->Composite( image=>$inputImage, gravity=>'NorthWest');
	$outputImage->Composite( image=>$downImage, gravity=>'SouthWest');
	$outputImage->Composite( image=>$valueImage, x=>-5);	# Pad it a little on the left.
	$outputImage->Composite( image=>$labelImage, x=>$outputWidth-150);

	# write out the image.
	$outputImageName=sprintf('%s/%05d.jpg',$annotatedImageDirectory,$fileNum);
	$outputImage->write($outputImageName);
}

# --------------------------------------------------------------------
#----------------------- Two pass MPEG encoding ----------------------
# --------------------------------------------------------------------
# First ffmpeg pass.
#if (system "ffmpeg -passlogfile ~/ffmpeg_$flightNumber -r $outputFrameRate -b $mp4BitRate -y -title $projectNumber$flightNumber -author 'S. Beaton NCAR/RAF' -pass 1 -i $annotatedImageDirectory/%05d.jpg ~/$flightNumber.mp4") {die "Unable to create MPEG file $flightNumber.mp4, pass 1"};
if (system "ffmpeg -passlogfile ./ffmpeg_$flightNumber -r $outputFrameRate -b $mp4BitRate -y -pass 1 -i $annotatedImageDirectory/%05d.jpg ./$flightNumber.mp4") {die "Unable to create MPEG file $flightNumber.mp4, pass 1"};

# Second pass.
#if (system "ffmpeg -passlogfile ~/ffmpeg_$flightNumber -r $outputFrameRate -b $mp4BitRate -y -title $projectNumber$flightNumber -author 'S. Beaton NCAR/RAF' -pass 2 -i $annotatedImageDirectory/%05d.jpg ~/$flightNumber.mp4") {die "Unable to create MPEG file $flightNumber.mp4, pass 2"};
if (system "ffmpeg -passlogfile ./ffmpeg_$flightNumber -r $outputFrameRate -b $mp4BitRate -y -pass 2 -i $annotatedImageDirectory/%05d.jpg ./$flightNumber.mp4") {die "Unable to create MPEG file $flightNumber.mp4, pass 2"};


# --------------------------------------------------------------------
#-------------------------------- END --------------------------------
# --------------------------------------------------------------------
print "Normal program Completion.\n";
