var capturePHP="/camera/capture.php"; //path from webroot to capture.php (update this manuall if it changes)
var camServer="http://sloop2.eol.ucar.edu"; //camserver (gets updated from db automatically)

//Global Vars
var imgSrc = new Array();
var flightNumber = 0;
var maxVal = -2, offsetTime = 0, imgsReady=0;
var rulers=false, grids=false;
var scale = "1024";
var now = new Date();
var d = new Date();

function getNames(offset, pbmode){
//get the name of the the image offset by <offset> seconds,
// pass pbmode to setImgs if offset != 0
	if (offset == 0) {
		$.getJSON("getimgs.php", function(data){
			setImgs(data.jsonImgs[$(".camCheck").size()-1], false);
		});
	} else {
		d.setTime(now.valueOf() + (1000*offset) + offsetTime);
		newImg = d.format("yymmdd-HHMMss") + ".jpg";
		setImgs(newImg, pbmode);
	}
}

function setImgs(name, pbmode){
//start loading the images for the next playback frame
	for (i=1; i<=$(".camCheck").size(); i=i+1){
		if ($("#showCam"+i).attr('checked') == 1 ){
			imgSrc[i]  = "flight_number_";
		 	imgSrc[i] += flightNumber;
			imgSrc[i] += "/";
			imgSrc[i] += $("#showCam"+i).attr('alt');
			imgSrc[i] += "/";
			imgSrc[i] += name;
	
			if (pbmode) {
				$('<img>')
					.load(function() {
						imgsReady++;
						})
					.error(function() {
						imgsReady++;
						})
					.attr('width', scale)
					.attr('src', imgSrc[i]);
			} else { 
				$("#cam"+i).attr("src", imgSrc[i]);
			}
			if(rulers){$("img.grad"+i).show();}
			if(grids){$("img.grid:eq("+(i-1)+")").show();}

		}else{
			$("#cam"+i).attr("src", "");
			$("img.grad"+i).hide();
			$("img.grid:eq("+(i-1)+")").hide();
		}	
	}	
		
	$("#imgNum").html(name);
}

function timedFunc() {
//main loop: updates the images and then calls itself
	if ( $("#autoUpdate").attr('checked') == 1) {
		now = new Date();
		getNames(0, false);
	}

	$("span[id='status']").load("status.php");
	if ($("#fdCheck").attr("checked") == true){
		$("span[id='flightData']").load("flightData.php");
	}
	setTimeout("timedFunc()", 1000);
}

function playFunc() {
//'Plays back' the archived images
	var curVal = $("#slider").slider('option', 'value');
	if( curVal <= maxVal ){
		if ($("#playButton").attr("checked") == true){
			imgsReady = 0;
			getNames(curVal, true);
			$("#slider").slider('option', 'value', curVal+5);
			setTimeout("checkReadyImgs()", 10);
		}
	} else {
		$("#playButton").attr("checked", false);
		$("#autoUpdate").attr("checked", true);
	}	
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

function autoscaler() {
//Determines and sets the new image scaling
	var auto = $("#autoScale").attr('checked');
	var sideways = $("#sideways").attr('checked');
	var ccams = $(".camCheck:checked").size();
    var menWidth = document.getElementById('flightData').offsetWidth;
	var menuWidth = menWidth == 0 ? 175 : menWidth;
	menuWidth += 100;

	if (auto) {

		if(sideways){
			if ((document.documentElement.clientWidth - menuWidth) < (document.documentElement.clientHeight * ccams )) {
				scale = (document.documentElement.clientWidth / ccams ) - (menuWidth/ccams);
			} else {
				scale = (document.documentElement.clientHeight );
			}
		} else {
			if (((document.documentElement.clientWidth - menuWidth) * ccams) < (document.documentElement.clientHeight)) {
				scale = (document.documentElement.clientWidth ) - menuWidth;
			} else {
				scale = (document.documentElement.clientHeight / ccams );
			}
		}
		$("img.cam").attr("width", scale);
		$("img.grid").attr("width", scale);

	} else {
		$("img.cam").removeAttr("width");
		for (i=1; i<=$(".camCheck").size(); i=i+1){
			$("img.grid:eq("+(i-1)+")").attr("width", ($("#cam"+i).attr('width') ));	
		}
	}

	for (i=1; i<=$(".camCheck").size(); i=i+1){
		$("img.grad"+i+":first").attr("height", ($("#cam"+i).attr('height') ));
	}
	if ($("#autoUpdate").attr('checked') == false) {
		getNames($("#slider").slider('value'),false);
	}
}

function buildImageTable(sideways) {
//sets up the HTML table that holds the images
	var table_contents=" ";
	if(sideways){
		table_contents = "<tr>";
		for (i=1; i<=$(".camCheck").size(); i=i+1){
			table_contents = table_contents 
				+ '<td><img src="vgrad.png" class="grad' + i
				+ '" width="10"/></td><td><img class="grid" src="grid.png" style="display:none;" /><img id="cam' + i
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
				+ '" width="10"/></td><td><img class="grid" src="grid.png" style="display:none;" /><img id="cam' + i
				+ '" class="cam" src="nothere.jpg" /></td></tr><tr><td></td><td><img src="grad.png" class="grad' + i
				+ '" height="10" width="100%" /></td></tr> \n';
		} 
	}
	$("#imageTable").html(table_contents);
	autoscaler(); //scale images
}

$(function(){
/* ************************************************
 * JQuery page load function,  This function runs 
 * when the page is loaded.
 * Need to put all JQuery setup and event handlers
 * inside this function
 **************************************************/

	/*=========    SET UP HTML   ============*/
	
	//capture status, get flight nubmer
	$("span[id='status']").load("status.php");

	$.getJSON("getData.php", function(data){
		//check for offset between local and server clock
		var offsetString = data.datetime;
		var servDate = new Date;
		servDate.setTime(offsetString);
		var locDate = new Date();
		offsetTime = servDate.valueOf() - locDate.valueOf();

		//update flight number from DB
		flightNumber = data.curFlNum;
		$("span[id='flightNum']").text(flightNumber);

		//get hostname of the camserver that last updated the database	
		camServer = 'http://' + data.chost;
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
			$("#autoUpdate").attr("checked", false);
			var mins=Math.floor(-ui.value/60);
			if (mins) {
				$("#imgNum").html(mins + " Minutes " + (-ui.value%60) + " Seconds Ago");
			} else {
				$("#imgNum").html((-ui.value%60) + " Seconds Ago");
			}
		},
		stop: function(event, ui) {
			getNames(ui.value, false)
		}
	});


	/*=========    SET UP EVENT HANDLERS    ============*/

	//autoscale events
	$(":checkbox:not('#fdCheck')").change(function() {
		autoscaler();
	});
	$(window).resize(function() {
		autoscaler();
	});

	//live flight data
	$("#fdCheck").change(function() {
		if ($("#fdCheck").attr('checked')) {
			$("#fdDiv").show();
		} else {
			$("#fdDiv").hide();
		}
	});

	//grid event
	$("#grids").change(function() {
		grids = $("#grids").attr('checked');
		if (!grids) {
			for (i=1; i<=$(".camCheck").size(); i=i+1){
				$("img.grid:eq("+(i-1)+")").hide();
			}
		}
	});					

	//rulers event
	$("#rulers").change(function() {
		rulers = $("#rulers").attr('checked');
		if (!rulers) {
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
		$.ajax({type: "GET", url: camServer+capturePHP+"?start=1", dataType: "jsonp"});
	});
	$("#stopRec").click(function(){
		$.ajax({type: "GET", url: camServer+capturePHP+"?stop=1", dataType: "jsonp"});
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
