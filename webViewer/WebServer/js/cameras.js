//Global Vars
var imgSrc = new Array();
var flightNumber = 0, curImg="start";
var maxVal = -2, offsetTime = 0, imgsReady=0;
var crossHairs=false;
var scale = "1024";
var now = new Date();
var d = new Date();

function getNames(offset){
//get the name of the the image offset by <offset> seconds
	d.setTime(now.valueOf() + (1000*offset) + offsetTime);
	newImg = d.format("yymmdd-HHMMss") + ".jpg";
	$("#imgNum").html(newImg);
	return newImg;
}

function getDbNames(){
//get the name of the latest image in the DB
//	$(".loading").show();
	$.get("getimgs.php", "cam=1", function(data){
		curImg = data;
		$("#imgNum").html(data);
//		$(".loading").hide();
		setImgs();
	});
}

function setImgs(){
//append the full file path and set the 'src' tags to the new images
	for (i=1; i<=$(".camCheck").size(); i=i+1){
		if ($("#showCam"+i).attr('checked') == 1 ){
			$("#cam"+i).attr("src", "flight_number_" + flightNumber + "/" + 
						$("#showCam"+i).attr('alt') + "/" + curImg);

			if(crossHairs){$("img.grad"+i).show();}
		}else{
			$("#cam"+i).attr("src", "");
			$("img.grad"+i).hide();
		}	
	}
}

function setImgs2(x){
//start loading the images for the next playback frame
	for (i=1; i<=$(".camCheck").size(); i=i+1){
		if ($("#showCam"+i).attr('checked') == 1 ){
			
			imgSrc[i]  = "flight_number_";
		 	imgSrc[i] += flightNumber;
			imgSrc[i] += "/";
			imgSrc[i] += $("#showCam"+i).attr('alt');
			imgSrc[i] += "/";
			imgSrc[i] += getNames(x);
	
			$('<img>')
				.load(function() {
					imgsReady++;
					})
				.error(function() {
					imgsReady++;
					})
				.attr('width', scale)
				.attr('src', imgSrc[i]);
		}
	}	
		
}

function timedFunc() {
//main loop: updates the images and then calls itself
	if ( $("#autoUpdate").attr('checked') == 1) {
		now = new Date();
		getDbNames();
	}

	$("span[id='status']").load("status.php");
	setTimeout("timedFunc()", 1000);
}

function checkReadyImgs() {
//check to see if all images are loaded, 
//if so, set image 'src', and start next frame (playFunc())
	var numChecked = $(".camCheck:checked").size();
	if (imgsReady < numChecked){
		$(".loading").show();
		setTimeout("checkReadyImgs()", 10);
	} else {	
		$(".loading").hide();
		for (i=1; i<=$(".camCheck").size(); i=i+1){
			if ($("#showCam"+i).attr('checked') == 1 ){
				$("#cam"+i).attr('src', imgSrc[i]);
			}
		}
		playFunc();
	}
}

function playFunc() {
//'Plays back' the archived images
	var curVal = $("#slider").slider('option', 'value');
	if( curVal <= maxVal ){
		if ($("#playButton").attr("checked") == true){
			imgsReady = 0;
			setImgs2(curVal);
			$("#slider").slider('option', 'value', curVal+5);
			setTimeout("checkReadyImgs()", 10);
		}
	} else {
		$("#playButton").attr("checked", false);
		$("#autoUpdate").attr("checked", true);
	}	
}

function autoscaler() {
//Determines and sets the new image scaling
	var auto = $("#autoScale").attr('checked');
	var sideways = $("#sideways").attr('checked');
	var ccams = $(".camCheck:checked").size();
	if (auto) {

		if(sideways){
			if ((document.documentElement.clientWidth - 250) < (document.documentElement.clientHeight * ccams )) {
				scale = (document.documentElement.clientWidth / ccams ) - (250/ccams);
			} else {
				scale = (document.documentElement.clientHeight );
			}
		} else {
			if (((document.documentElement.clientWidth - 250) * ccams) < (document.documentElement.clientHeight)) {
				scale = (document.documentElement.clientWidth ) - 250;
			} else {
				scale = (document.documentElement.clientHeight / ccams );
			}
		}
		$("img.cam").attr("width", scale);

	} else {
		$("img.cam").removeAttr("width");
	}

	for (i=1; i<=$(".camCheck").size(); i=i+1){
		$("img.grad"+i+":first").attr("height", ($("#cam"+i).attr('height') ));
	}
	setImgs();
}

function buildImageTable(sideways) {
//sets up the HTML table that holds the images
	var table_contents=" ";
	if(sideways){
		table_contents = "<tr>";
		for (i=1; i<=$(".camCheck").size(); i=i+1){
			table_contents = table_contents 
				+ '<td><img src="vgrad.png" class="grad' + i
				+ '" width="10"/></td><td><img id="cam' + i
				+ '" class="cam" src="nothere.jpg" /></td> \n';
		}
		table_contents = table_contents + '</tr><tr>';
		for (i=1; i<=$(".camCheck").size(); i=i+1){
			table_contents = table_contents 
				+ '<td></td><td><img src="grad.png" class="grad' + i
				+ '" height="10" width="100%" /></td> \n';
		}
		table_contents = table_contents + '</tr>';

	} else {
		for (i=1; i<=$(".camCheck").size(); i=i+1){
			table_contents = table_contents
				+ '<tr> <td><img src="vgrad.png" class="grad' + i
				+ '" width="10"/></td><td><img id="cam' + i
				+ '" class="cam" src="nothere.jpg" /></td></tr><tr><td></td><td><img src="grad.png" class="grad' + i
				+ '" height="10" width="100%" /></td></tr> \n';
		} 
	}
	$("#imageTable").html(table_contents);
	autoscaler(); //scale images
}

$(function(){
//This function runs when the page is loaded

	/*=========    SET UP HTML   ============*/
	
	//capture status, get flight nubmer
	$("span[id='status']").load("status.php", "first=yes", function(){
		//check for offset between local and server clock
		var offsetString = $("#datetime").text();
		var servDate = new Date;
		servDate.setTime(offsetString);
		var locDate = new Date();
		offsetTime = servDate.valueOf() - locDate.valueOf();

		//update flight number from DB
		flightNumber = $("#curFlNum").text();
		$("span[id='flightNum']").text(flightNumber);
	});

	//load camera selector checkboxes (get names from DB)
	$('#camSelectors').load('camSelect.php', function(){
		buildImageTable( true ); //set up image table
		timedFunc();  //start showing/refreshing images	
	});

	//set time scale for slider
	$("#sliderMin").change(function() {
		var newTime = (-60 * $("#sliderMin").attr('value'));
		$("#slider").slider('option', 'min', newTime);
	});

	//set up tabs
	$("#tabs").tabs();		

	//set up Slider
	$('#slider').slider({
		min: -300,
		max: maxVal,
		step: 1,
		slide: function(event, ui) {
			curImg = getNames(ui.value);
			$("#autoUpdate").attr("checked", false);
		},
		stop: function(event, ui) {
			setImgs();
		}
	});


	/*=========    SET UP EVENT HANDLERS    ============*/

	//autoscale events
	$(":checkbox").change(function() {
		autoscaler();
	});
	$(window).resize(function() {
		autoscaler();
	});

	//crosshairs event
	$("#xHairs").change(function() {
		crossHairs = $("#xHairs").attr('checked');
		if (!crossHairs) {
			for (i=1; i<=$(".camCheck").size(); i=i+1){
				$("img.grad"+i).hide();
			}
		}
	});					

	//request horizonal image layout
	$("#sideways").change(function() {
		buildImageTable($("#sideways").attr('checked'));	
	});
	
	//start,stop, refresh buttons
	$("#startRec").click(function(){
		$("#hiddenDiv").load("capture.php", "start=yes");
		$("#status").load("status.php", "first=yes", function() {
			flightNumber = $("#curFlNum").text();
			//$("#output").load("output.php");
		});
	});
	$("#stopRec").click(function(){
		$("#hiddenDiv").load("capture.php", "stop=yes");
	});
	$("#manRefresh").click(function(){
		now = new Date();
		$("#slider").slider('value', maxVal);
		getDbNames();
	});	

	//live or playback control
	$("#autoUpdate").change(function() {
		if ($("#autoUpdate").attr("checked")) {
			$("#playButton").attr("checked", false);
			$("#slider").slider('value', maxVal);
		}
	});

	$("#playButton").change(function(){
		if ($("#playButton").attr("checked")){
			$("#autoUpdate").attr("checked", false);
			if ( $("#slider").slider('option', 'value') >= maxVal){
				$("#slider").slider('option', 'value', (-60 * $("#sliderMin").attr('value')));
			}
			playFunc();		
		}
	});

});
