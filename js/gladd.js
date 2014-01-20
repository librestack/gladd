/* 
 * gladd.js - gladd helper functions
 *
 * this file is part of GLADD
 *
 * Copyright (c) 2012, 2013, 2014 Brett Sheffield <brett@gladbooks.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (see the file COPYING in the distribution).
 * If not, see <http://www.gnu.org/licenses/>.
 */

var g_authurl = '/auth/';
var g_url_form = '/html/forms/';
var g_authtimeout = 60000; /* milliseconds - every 60s */
var g_username = '';
var g_password = '';
var g_instance = '';
var g_business = '1';
var g_loggedin = false;
var g_tabid = 0;
var g_max_tabtitle = '48'; /* max characters to allow in tab title */
var g_session = false;
var TABS = new Tabs();
var g_timeout = 10000; /* timeout for ajax requests */

var STATUS_INFO = 1;
var STATUS_WARN = 2;
var STATUS_CRIT = 4;

/*****************************************************************************/
$(document).ready(function() {
	/* no password, display login dialog */
	if (g_password == '') { displayLoginBox(); }

	/* prepare tabbed workarea */
	deployTabs();

	/* prepare menu */
	prepMenu();

	/* reload when logo clicked */
	$("img#logo").click(function(event) {
		event.preventDefault();
		dashboardShow();
	});     

	loginSetup();

	$('button.submit').click(function() {
		// grab those login details and save for later
		g_username = $('input:text[name=username]').val();
		g_password = $('input:password[name=password]').val();
		auth_check();
	});
	
	$(window).unload(function() {
		logout();
	});

	$(document).keypress(docKeypress);

    siteEvents();
});

/* attach delegated site-wide events */
function siteEvents() {
    console.log('siteEvents()');
    var site = $('div#sitediv');

    site.on('click', 'a.tablink', function(e) {
        e.preventDefault();
        clickTabLink(this);
        return false;
    });
}

/* to be overridden in app */
function docKeypress() {
}

function loginSetup() {
	/* set up login box */
	$("form.signin :input").bind("keydown", function(event) {
		// handle enter key presses in input boxes
		var keycode = (event.keyCode ? event.keyCode :
			(event.which ? event.which : event.charCode));
		if (keycode == 13) { // enter key pressed
			// submit form
			document.getElementById('btnLogin').click();
			event.preventDefault();
		}
	});
}

/******************************************************************************
 * auth_check()
 *
 * Request an auth required page to test login credentials.
 * If successful, we can consider this user logged in.
 * Else, chuck them back to the login page with an error
 * NB: we send Authorization: 'Silent' instead of 'Basic' to 
 * prevent the browser popping up an auth dialog.
 *
 *****************************************************************************/
function auth_check()
{
	$.ajax({
		url: g_authurl,
		timeout: g_timeout,
		beforeSend: function (xhr) { setAuthHeader(xhr); },
		success: function(data) { loginok(data); },
		error: function(data, s, e) {
			if (t == 'timeout') {
				console.log('auth_check() timeout.  Retrying.');
				auth_check();
			}
			else {
				loginfailed();
			}
		}
	});
}

function auth_session_logout(async)
{
	console.log('session logout...');
	if (async == undefined) { async = true }; /*run asynchronously by default*/
	g_session = false;
	return $.ajax({
		url: g_authurl,
		async: async,
		beforeSend: function (xhr) { 
			setAuthHeader(xhr);
			xhr.setRequestHeader("Logout", g_username);
		},
		complete: function(data) { 
			console.log('session logout complete');
		},
	});
}

/*****************************************************************************/
/* Prepare authentication hash */
function auth_encode(username, password) {
	var tok = username + ':' + password;
	var hash = Base64.encode(tok);
	return hash;
}

/*****************************************************************************/
/* prepare tabbed workarea */
function deployTabs() {
	$('.tabcloser').click(function(event) {
		event.preventDefault();
		TABS.byId[$(this).attr('href')].close();
	});
}

/*****************************************************************************/
/* add a new tab with content, optionally activating it */
function addTab(title, content, activate, collection, refresh) {
	console.log('addTab()');
	var tab = new Tab(title, content, activate, collection, refresh);
	return tab.id;
}

/* find tab by title within active business */
function getTabByTitle(title) {
	var tabid = $('li.tabhead.business' + g_business).find('a.tabtitle:contains("' + title + '")').attr("href");
	console.log('Found tab by title: ' + tabid);
	return tabid;
}

/*****************************************************************************/
function activeTabId() {
	return $('li.tabhead.active.business' + g_business
		).find('a.tabcloser').attr('href');
}

/*****************************************************************************/
/* o can either be the id of a tab, or a jquery object */
function updateTab(o, content, activate, title) {
	console.log('updateTab()');
	/* figure out what o represents */
	if (isTabId(o)) { 				/* numeric */
		var tab = TABS.byId[o];
		tab.setContent(content);
		tab.setTitle(title);
		if (activate) { tab.activate(); }
	}
	else if (typeof o == 'object'){ /* jquery object (probably) */
		o.empty().append(content);
	}
	else { 							/* unexpected type */
		console.log(typeof o + ' passed to updateTab()');
		return false;
	}

	return o;
}

/******************************************************************************
 * return jQuery object for specified tab, or active tab if no tab specified */
function getTabById(tabid) {
	if (tabid == null) {
		return activeTab();
	}
	if (isTabId(tabid)) {
		return $('#tab' + tabid);
	}
	else {
		return tabid;
	}
}

/*****************************************************************************/
function activateTab(tabid) {
	if (isTabId(tabid)) {
		TABS.byId[tabid].activate();
	}
}



/*****************************************************************************/
/* remove a tab */
function closeTab(tabid) {
	var tabcount = $('div#tabs').find('div').size();

	if (! tabid) {
		/* close the active tab by default */
		var tabid = activeTabId();
	}

	/* if tab is active, activate another */
	if ($('.tablet' + tabid).hasClass('active')) {
		console.log("tab (" + tabid  + ") was active");
		activateNextTab(tabid);
	}

	/* remove tab and content - call me in the morning if pain persists */
	$('.tablet' + tabid).remove();

	/* if we have tabs left, fade out */
	if (tabcount == 1) {
		$('div#tabs').fadeOut(300);
	}
}

/*****************************************************************************/
/* Remove all tabs from working area */
function removeAllTabs() {
	$('ul.tablist').children().remove(); /* tab headers */
	$('div.tablet').fadeOut(300);		 /* content */
}

/*****************************************************************************/
/* Add Authentication header with logged-in user's credentials */
/* only send if no session active */
function setAuthHeader(xhr, username, password) {
	if (username == undefined) { username = g_username; }
	if (password == undefined) { password = g_password; }
	if (!g_session) {
		console.log('setting auth header');
		var hash = auth_encode(username, password);
		xhr.setRequestHeader("Authorization", "Silent " + hash);
	}
	else {
		console.log('NOT setting auth header');
	}
}

/* If we're still logged in, refresh our session cookie */
function reauthenticate() {
    if (g_loggedin) {
        console.log('refreshing session cookie');
        g_session = false;
        auth_check();
    }
}

/*****************************************************************************/
/* login successful, do successful things */
function loginok(xml) {
	g_session = true;

	/* set timer to re-authenticate, so we always have a fresh cookie */
    console.log('setting auth timer');
    window.setTimeout(reauthenticate, g_authtimeout);

	customLoginEvents(xml);
}

/* to be overridden by application */
function customLoginEvents(xml) {
}

/*****************************************************************************/
/* Login failed - inform user */
function loginfailed() {
	g_password = '';
	g_loggedin = false;
	alert("Login incorrect");
	setFocusLoginBox();
}

/*****************************************************************************/
/* logout() - Clear password and mark user logged out.  */
function logout()
{
	/* remove user menus */
	dropMenu();

	/* clear business selector */
	select = $('select.businessselect');
	select.empty();
	select.append('<option>&lt;select business&gt;</option>');

	/* clear working area */
	removeAllTabs();

	/* send logout to server, clearing the session */
	auth_session_logout();

	/* clear password */
	g_password = '';
	g_loggedin = false;
	$('input:password[name=password]').val('');

	customLogoutActions();

	displayLoginBox();
}

/* to be overridden by application */
function customLogoutActions() {
}

/******************************************************************************
 * displayLoginBox()
 *
 * Display login dialog.  
 *
 * Based on:
 *   http://www.alessioatzeni.com/blog/login-box-modal-dialog-window-with-css-and-jquery/
 */
function displayLoginBox() {
	var loginBox = "#login-box";

	// we have the username already - grab it so focus is set properly later
	g_username = $('input:text[name=username]').val();

	// Fade in the Popup, setting focus when done
	$(loginBox).fadeIn(300, function () { setFocusLoginBox(); });
	
	// Set the center alignment padding + border see css style
	var popMargTop = ($(loginBox).height() + 24) / 2; 
	var popMargLeft = ($(loginBox).width() + 24) / 2; 
	
	$(loginBox).css({ 
		'margin-top' : -popMargTop,
		'margin-left' : -popMargLeft
	});
	
	// Fade in background mask unless already visible
	$('#mask').fadeIn(300);

};

/*****************************************************************************/
/* Set Focus in Login Dialog Appropriately */
function setFocusLoginBox() {
	// if username is blank, set focus there, otherwise set it to password
	if (g_username == '') {
		$("#username").focus();
	} else {
		$("#password").focus();
	}
};

/*****************************************************************************/
/* Hide Login Dialog */
function hideLoginBox() {
	$('#mask , .login-popup').fadeOut(300 , function() {
		$('#mask').hide();  
	}); 
}

/*****************************************************************************/
/* prepare static menus */
function prepMenu() {
	$('ul.nav').find('a').each(function() {
		$(this).click(clickMenu);
	});
}

/*****************************************************************************/
/* Fetch user specific menus in xml format */
function getMenu() {
	$.ajax({
		url: g_authurl + g_username +  ".xml",
		beforeSend: function (xhr) { setAuthHeader(xhr); },
		success: function(xml) { setMenu(xml); },
		error: function(xml) { setMenu(xml); }
	});
}

/*****************************************************************************/
function dropMenu() {
	/* move Logout out of the way */
	$logout = $('a#logout').detach();

	/* delete the rest of the menu contents */
	$("div#menudiv").empty();

	/* put Logout back, but clear */
	$("div#menudiv").append($logout);
	$('a#logout').text('');
}

/*****************************************************************************/
function setMenu(xml) {
	/* move Logout out of the way */
	$logout = $('a#logout').detach();

	/* load xml with user's menus */
	$(xml).find("login").find("menu").each(function() {
		var item = $(this).attr("item");
		var tip = $(this).attr("tooltip");
		var href = $(this).attr("href");
		var n = $('<a href="'+ href +'" title="'+ tip +'">'+ item +'</a>');
		$(n).on("click", { url: href }, clickMenu);
		$("div#menudiv").append(n);
	});

	/* finally, add back Logout menu item */
	$("div#menudiv").append($logout);
	$('a#logout').text('Logout (' + g_username  + ')' );
}

/*****************************************************************************/
/* grab menu event and fetch content in the background */
function clickMenu(event) {
    event.preventDefault();

    ok = false;

    console.log('clickMenu()');

	if ($(this).attr("href") == '#') {
		console.log('Doing nothing, successfully');
		ok = true;
	}
	else {
		statusHide(); /* clear any previous error messages from status box */
		for (menu in g_menus) {
				if ($(this).attr("href") == '#' + g_menus[menu][0]) {
						if (g_menus[menu][1] == alert ) {
							alert(g_menus[menu][2]);
						}
						else {
							g_menus[menu][1].apply(this, Array.prototype.slice.call(g_menus[menu], 2));
						}
						ok = true;
						break;
				}
		}
	}
    if (!ok) {
        addTab("null", "<h2>Feature Not Available Yet</h2>", true);
    }
}

function clickTabLink(ctl) {
    var url = $(ctl).attr("href");
    console.log('clickTabLink(' + url + ')');
	showHTML(url, 'Help' , false);
    return ctl;
}

/*****************************************************************************/
/* Display query results as list */
function showQuery(collection, title, sort, tab) {
	$.ajax({
		url: collection_url(collection),
		timeout: g_timeout,
		beforeSend: function (xhr) { setAuthHeader(xhr); },
		success: function(xml) {
			displayResultsGeneric(xml, collection, title, sort, tab);
		},
		error: function(xml) {
			if (tab) {
				if (tab.children().length > 0) {
					/* tab has contents - leave them alone on failure */
					console.log('failed to load ' + collection 
						+ '.  Retrying.');
					showQuery(collection, title, sort, tab);
					return false;
				}
			}
			displayResultsGeneric(xml, collection, title, sort, tab);
		}
	});
}

/*****************************************************************************/
/* Fetch HTML fragment and display in new tab */
function showHTML(url, title, div, collection) {
	console.log('showHTML()');
	if (collection) url = collection_url(url);
	showSpinner();
	return $.ajax({
		url: url,
		beforeSend: function (xhr) { setAuthHeader(xhr); },
		success: function(html) {
			hideSpinner();
			if (div) {
				updateTab(div, html);
			}
			else {
				addTab(title, html, true);
			}
			//accordionize(activeTab().find('div.accordion'));
		},
		error: function() {
			if (div) {
				updateTab(div, 'Not found.');
			}
			else {
				addTab(title, 'Not found.', false);
			}
			hideSpinner();
		}
	});
}

/******************************************************************************/
/* fetch html form from server to display */
/* TODO: clean up - xml arg no longer needed? */
function getForm(object, action, title, xml, tab) {
	console.log('getForm(' + object + ',' + action + ',' + title + ')');
	showSpinner(); /* tell user to wait */

	var d = fetchFormData(object, action);
	return $.when.apply(null, d)
	.done(function(html) {
		var args = Array.prototype.splice.call(arguments, 1);
		var safeHTML = $.parseHTML(html[0]);
		cacheData(object, action, args); /* FIXME: remove */
		displayForm(object, action, title, safeHTML, args, tab);
	})
	.fail(function() {
		console.log('fetchFormData() failed');
		hideSpinner();
	});
}

function cacheData(object, action, args) {
	console.log('cacheData(' + object + ',' + action + ')');
	if ((object == 'account') && (action == 'create')) {
		g_xml_accounttype = args[0];
	}
}


/*****************************************************************************/
function fetchFormData(object, action) {
    console.log('fetchFormData(' + object + ',' + action + ')');
    var d = new Array(); /* array of deferreds */
    var ok = false;

    var formURL = '/html/forms/' + object + '/' + action + '.html';
    d.push(getHTML(formURL));   /* fetch html form */

    for (o in g_formdata) {
        if ((object == g_formdata[o][0]) && (action == g_formdata[o][1])) {
            /* loop through and push all sources */
            for (i=0; i <= g_formdata[o][2].length-1; i++) {
                d.push(getXML(collection_url(g_formdata[o][2][i])));
            }
            ok = true;
        }
    }

    if (!ok) {
        d.push($.Deferred().resolve()); /* succeed at nothing */
    }

    return d;
}

/******************************************************************************/
/* return active tab as jquery object */
function activeTab() {
	return $('div.tablet.active.business' + g_business);
}

/*****************************************************************************/
/* pre-populate form with xml data                                           */
function populateForm(tab, object, xml) {
	var geodata = new Array();
	var id = '';
	var locString = '';
	var mytab = getTabById(tab);

	if (xml) {
		/* we have some data, pre-populate form */
		$(xml[0]).find('resources').find('row').children().each(function() {
			var tagName = this.tagName;
			var tagValue = $(this).text();

			if ((tagName == 'id') || (tagName == object)) {
				id = tagValue;
			}

			console.log(tagName + '=' + tagValue);

			/* set field value */
			var fld = mytab.find('form.'+ object +" [name='"+ tagName +"']");
			fld.val(tagValue);
			fld.data('old', tagValue);/* keep a note of the unmodifed value */
			fld.trigger("liszt:updated");/* ensure chosen type combos update */

			/* get location data, giving preference to postcode */
			if ([
				'line_1',
				'line_2',
				'line_3',
				'town',
				'county',
				'postcode',
				'country',
			].indexOf(tagName) != -1 && tagValue.length > 0) {
				geodata.push(tagValue);
			}
		});

		/* load map */
		if (geodata.length > 0) {
			var locString = geodata.join();
			console.log('locString:' + locString);
			loadMap(locString, tab);
		}
	}
	else {
		console.log('No data to populate form.');
	}
	return id;
}

/*****************************************************************************/
/* deal with subforms */
function handleSubforms(tab, html, id, xml) {
	console.log('handleSubforms()');
	$(html).find('form.subform').each(function() {
		var view = $(this).attr("action");
		var mytab = getTabById(tab);
		var datatable = mytab.find('div.' + view).find('table.datatable');
		var btnAdd = datatable.find('button.addrow:not(.btnAdd)');
		btnAdd.click(function(){
			if (view == 'salesorderitems') {
				salesorderAddProduct(tab, datatable);
			}
			else {
				addSubformEvent($(this), view, id, tab);
			}
		});
		btnAdd.addClass('btnAdd');
		if (id) {
			displaySubformData(view, id, xml, tab);
		}
	});
}

/* this function is intended to be overridden by the application */
function tabTitle(title) {
	return title;
}

function isTabId(o) {
	if (typeof o == 'number') {
		return true;
	}
	else if (typeof o == 'string') {
		return !(isNaN(o));
	}
	return false;
}

function addOrUpdateTab(container, content, activate, title) {
    if (typeof container == 'undefined') {
		console.log('no container => new tab');
		var tab = new Tab(title, content, activate);
	}
	else {
		console.log('container => update tab');
		var id = updateTab(container, content, activate, title);
		var tab = TABS.byId[id];
	}
	return tab;
}

/*****************************************************************************/
/* display html form we've just fetched in new tab */
function displayForm(object, action, title, html, xml, container) {
	console.log('displayForm("'+ object +'","'+ action +'","'+ title +'")');

	var title = tabTitle(title, object, action, xml);
	var tab = addOrUpdateTab(container, html, true, title);

	/* add some metadata */
	tab.object = object;
	tab.action = action;

	var mytab = tab.tablet;

	/* populate combos with xml data */
	/* NB: on update, skip first xml as this contains the record itself */
	var x = (action == 'update') ? 1 : 0; /* which xml to start with */
	//mytab.find('select.populate:not(.sub)').populate(mytab);
	mytab.find('select.populate:not(.sub)').each(function() {
		populateCombo(xml[x++], $(this), tab.id);
	});

	/* pre-populate form */
	var id = (action == 'update') ? populateForm(tab.id, object, xml) : 0;

	/* deal with subforms and finalise form display */
	handleSubforms(tab.id, html, id, xml);
	finaliseForm(tab.id, object, action, id);
}

/*****************************************************************************/
function finaliseForm(tab, object, action, id) {
	console.log('finaliseForm()');
	formatDatePickers(tab);        			  /* date pickers */
	formatRadioButtons(tab, object);  		  /* tune the radio */
	formBlurEvents(tab);					  /* set up blur() events */
	formEvents(tab, object, action, id);      /* submit and click events etc. */
	activateTab(tab);			              /* activate the tab */
	hideSpinner();                            /* wake user */
}

/*****************************************************************************/
function formEvents(tab, object, action, id) {
	var mytab = getTabById(tab);

	/* save button click handler */
	mytab.find('button.save').click(function() 
	{
		doFormSubmit(object, action, id);
	});

	/* Cancel button closes tab */
	mytab.find('button.cancel').click(function()
	{
		TABS.active.close();
	});

	/* Reset button resets form */
	mytab.find('button.reset').click(function(event)
	{
		event.preventDefault();
		mytab.find('form:not(.subform)').get(0).reset();
	});

	/* deal with submit event */
	mytab.find('form:not(.subform):not(.nosubmit)').submit(function(event)
	{
		event.preventDefault();
		doFormSubmit(object, action, id);
	});

	customFormEvents(tab, object, action, id);
}

/* to be overridden by application */
function customFormEvents(tab, object, action, id) {
}

/* upload file, calling function f on success */
function uploadFile(f, url) {
	var form = new FormData(document.forms.namedItem("fileinfo"));

	showSpinner('Uploading File...');
	$.ajax({
	    url: url,
		type: 'POST',
		data: form,
		processData: false,
		contentType: false,
		beforeSend: function (xhr) { setAuthHeader(xhr); },
		success: function(xml) {
			if (f) {
				f(xml);
			}
			else {
				hideSpinner();
			}
		},
		error: function(xml) {
			hideSpinner();
			alert("error uploading file");
		}
	});
}

/* remove first row of xml document */
function stripFirstRow(xml) {
	$(xml).find('row:first').each(function() {
		$(this).remove();
	});
	return xml;
}

function fixXMLDates(xml) {
	$(xml).find('transactdate').each(function() {
		/* format date */
		var mydate = moment($(this).text());
		if (mydate.isValid()) {
			$(this).replaceWith('<' + this.tagName + '>' + mydate.format('YYYY-MM-DD') + '</' + this.tagName + '>');
		}
	});

	return xml;
}

/* repackage xml as request */
function fixXMLRequest(rawxml) {
	
	/* create a new XML request document */
	var doc = createRequestXmlDoc(data);

	/* clone the <data> part of our raw xml */
	var data = $(rawxml).find('resources').children().clone();

	/* paste the data into the new request xml */
	$(doc).find('data').each(function() {
		$(this).append(data);
	});

	/* flatten the xml ready to POST */
	//xml = flattenXml(doc);

	return xml;
}

function createRequestXmlDoc() {
    var doc = document.implementation.createDocument(null, null, null);

    function X() {
        var node = doc.createElement(arguments[0]), text, child;

        for(var i = 1; i < arguments.length; i++) {
            child = arguments[i];
            if(typeof child == 'string') {
                child = doc.createTextNode(child);
            }
            node.appendChild(child);
        }

        return node;
    };

    /* create the XML structure recursively */
    xml = X('request',
        X('instance', g_instance),
        X('business', g_business),
        X('data')
    );

	return xml;
}

function flattenXml(xml) {
	return (new XMLSerializer()).serializeToString(xml);
}

/*****************************************************************************/
function formBlurEvents(tab) {
	customBlurEvents(tab);
}

/* to be overridden by application */
function customBlurEvents(tab) {
}

/*****************************************************************************/
function formatRadioButtons(tab, object) {
	var mytab = getTabById(tab);
	/* tune the radio */
	mytab.find('div.radio.untuned').find('input[type="radio"]').each(function()
	{
		var oldid = $(this).attr('id'); /* note old id */
		$(this).attr('id', '');			/* remove old id */
		$(this).uniqueId(); 			/* add new, unique id */
		/* use old id to locate linked label, and update its "for" attr */
		$(this).parent().find('label[for="' + oldid + '"]').attr('for', 
			$(this).attr('id'));
	});
	mytab.find('div.radio.untuned').buttonset();
	mytab.find('div.radio.untuned').change(function() {
		changeRadio($(this), object);
	});
	mytab.find('div.radio.untuned').removeClass('untuned');
}

/*****************************************************************************/
function formatDatePickers(tab) {
	var mytab = getTabById(tab);
	mytab.find('.datefield').datepicker({
	   	dateFormat: "yy-mm-dd",
		constrainInput: true
	});
}

/*****************************************************************************/
function contactSearch(searchString) {
	console.log('searching for contacts like "' + searchString + '"');
	searchQuery('contacts', searchString);
}

/*****************************************************************************/
function searchQuery(view, query) {
    console.log('Loading subform with data ' + view);
    url = collection_url('search/' + view);
    if (query) {
        url += query + '/'
    }
    $.ajax({
        url: url,
        beforeSend: function (xhr) { setAuthHeader(xhr); },
        success: function(xml) {
            displaySearchResults(view, query, xml);
        },
        error: function(xml) {
            console.log('Error searching.');
        }
    });
}

/*****************************************************************************/
/* TODO */
function displaySearchResults(view, query, xml) {
	console.log("Search results are in.");
	var results = new Array();
	$(xml).find('row').each(function() {
		var id = $(this).find('id').text();
		var name = $(this).find('name').text();
		results.push(name);
	});
	activeTab().find('input.contactsearch').autocomplete(
		{ source: results }
	);
}

/*****************************************************************************/
/* a radio button value has changed */
function changeRadio(radio, object) {
	var station = radio.find('input[type="radio"]:checked').val();
	console.log('Radio tuned to ' + station);
	if (object == 'organisation') {
		if (station == '0') {
			$('tr.contact.link').hide();
			$('tr.contact.create').show();
		}
		else if (station == '1') {
			$('tr.contact.create').hide();
			$('tr.contact.link').show();
		}
	}
}

/*****************************************************************************/
function doFormSubmit(object, action, id) {
	console.log('doFormSubmit(' + object + ',' + action + ',' + id + ')');
	if (validateForm(object, action, id)) {
		if (id > 0) {
			submitForm(object, action, id);
		}
		else {
			submitForm(object, action);
		}
	}
}

/*****************************************************************************/
function validateForm(object, action, id) {
    console.log('validating form ' + object + '.' + action);
	statusHide(); /* remove any prior status */

	return customFormValidation(object, action, id);
}

/* to be overridden by application */
function customFormValidation(object, action, id) {
	return true;
}


/*****************************************************************************/
function statusHide() {
	var statusmsg = activeTab().find('div.statusmsg');
	statusmsg.hide();
}

/*****************************************************************************/
function statusMessage(message, severity, fade) {
	var statusmsg = activeTab().find('div.statusmsg');

	if (statusmsg.length == 0) {
		/* no status box, create one */
		console.log("No div.statusmsg - creating one");
		activeTab().find('div').first().prepend('<div class="statusmsg clearfix"/>');
		var statusmsg = activeTab().find('div.statusmsg');
	}

	statusmsg.removeClass('info warn crit');

	if (severity == STATUS_INFO) {
		statusmsg.addClass('info');
	}
	else if (severity == STATUS_WARN) {
		statusmsg.addClass('warn');
	}
	else if (severity == STATUS_CRIT) {
		statusmsg.addClass('crit');
	}
	
	statusmsg.text(message);
	statusmsg.show();

	if (fade) {
		statusmsg.fadeOut(fade);
	}
}

/*****************************************************************************/
/* comma separate thousands
 * TODO: make locale dependent */
function formatThousands(x) {
	var parts = x.toString().split(".");
	parts[0] = parts[0].replace(/\B(?=(\d{3})+(?!\d))/g, ",");
	return parts.join(".");
}

/*****************************************************************************/
function cloneInput(mytab, input, value) {
	console.log('Cloning input ' + input);
	if (input == 'total') {
		var iClass = 'clone';
	}
	else {
		var iClass = 'nosubmit';
	}
	var iSelector = 'input.' + iClass + '.' + input;
	var iBox = mytab.find(iSelector);
	var td = iBox.parent().clone(true);
	td.addClass('xml-' + input);
	td.find(iSelector).each(function() {
		$(this).val(value);
		$(this).removeClass(iClass);
		if (input == 'qty') {
			$(this).addClass('endsub');
		}
	});
	return td;
}

/*****************************************************************************/
function populateCombos(tab) {
	console.log('populateCombos()');
	var combosity = new Array();
	var mytab = getTabById(tab);

	mytab.find('select.populate:not(.sub)').each(function() {
		var combo = $(this);
		$(this).parent().find('a.datasource').each(function() {
			datasource = $(this).attr('href');
			console.log('datasource: ' + datasource );
			/* FIXME: remove or update caching */
			if ((datasource == 'relationships') && (g_xml_relationships)) {
				/* here's one we prepared earlier */
				console.log('using cached relationship data');
				populateCombo(g_xml_relationships, combo);
			}
			else {
				combosity.push(loadCombo(datasource, combo));
			}
		});
	});

	console.log('combosity level is ' + combosity.length);
	return $.when(combosity);
}

/*****************************************************************************/
function loadCombo(datasource, combo) {
	console.log('loadCombo()');
	url = collection_url(datasource);
	console.log('populating a combo from datasource: ' + url);

	return $.ajax({
		url: url,
		beforeSend: function (xhr) { setAuthHeader(xhr); },
		success: function(xml) {
			if (datasource == 'relationships') {
				console.log('caching relationship data');
				g_xml_relationships = xml;
			}
			populateCombo(xml, combo);
		},
		error: function(xml) {
			console.log('Error loading combo data');
		}
	});
}

/* a simpler combo population function */
$.fn.populate = function(xml) {
	$(this).each(function() { /* wrapped in case many objects selected */
		$(this)._populate(xml);
	});
}
$.fn._populate = function(xml) {
	console.log('$.populate()');
	var combo = $(this);
	var selections = [];

	/* first, preserve selections */
	for (var x=0; x < combo[0].options.length; x++) {
		selections[combo[0].options[x].text] = combo[0].options[x].selected;
	}

	combo.empty();

	/* add placeholder */
	if (combo.attr('data-placeholder') && combo.attr('multiple') != true) {
		console.log('combo has placeholder');
		var placeholder = combo.attr('data-placeholder');
		combo.append($("<option />").val(-1).text(placeholder));
	}

    /* now, repopulate and reselect options */
    $(xml).find('row').each(function() {
        var id = $(this).find('id').text();
        if (!isNaN(id)) {
            id = String(parseInt(id, 10)); /* strip leading zeros */
        }
        var name = $(this).find('name').text();
        combo.append($("<option />").val(id).text(name));
        if (selections[id] !== undefined) {
            combo[0].options[id].selected = selections[id];
        }
    });
}

/*****************************************************************************/
function populateCombo(xml, combo, tab) {
	console.log('populateCombo()');
	console.log('Combo data loaded for ' + combo.attr('name'));
	if (!(xml)) {
		console.log('no data supplied for combo');
		return false;
	}
	var selections = [];
	var mytab = isTabId(tab) ? getTabById(tab) : tab;

	/* first, preserve selections */
	for (var x=0; x < combo[0].options.length; x++) {
		selections[combo[0].options[x].text] = combo[0].options[x].selected;
	}

	combo.empty();

	/* add placeholder */
	if ((combo.attr('data-placeholder')) && (combo.attr('multiple') != true)) {
		console.log('combo has placeholder');
		var placeholder = combo.attr('data-placeholder');
		combo.append($("<option />").val(-1).text(placeholder));
	}

	/* now, repopulate and reselect options */
	$(xml).find('row').each(function() {
   		var id = $(this).find('id').text();
		var name = $(this).find('name').text();
		combo.append($("<option />").val(id).text(name));
		if (selections[id]) {
			combo[0].options[id].selected = selections[id];
		}
	});

	combo.chosen();

	if (combo.attr('name') == 'type') {
		/* add change() event to nominal code input box */
		mytab.find('input.nominalcode').change(function() {
			return validateNominalCode($(this).val(), combo.val());
		});
		combo.change(function() {
			comboChange($(this), xml, tab);
		});
		/* remove superfluous tab stop from chosen combo */
		$('div.chzn-container ul').attr("tabindex", -1);
	}
	else if (combo.attr('name') == 'account') {
		console.log('setting value of product->account combo');
		mytab.find('input.nosubmit[name="account"]').each(function() {
			combo.find(
				'option[value=' + $(this).val() + ']'
			).attr('selected', 'selected');
		});
	}
	else {
		combo.change(function() {
			comboChange($(this), xml, tab);
		});
	}

	combo.trigger("liszt:updated");
}

/*****************************************************************************/
/* handle actions required when combo value changes */
function comboChange(combo, xml, tab) {
	customComboChange(combo, xml, tab);
}

/* to be overridden by application */
function customComboChange(combo, xml, tab) {
}

/*****************************************************************************/
/* Fetch data for a subform */
function loadSubformData(view, id, tab) {
	console.log('loadSubformData()');
	console.log('Loading subform with data ' + view);
	url = collection_url(view);
	if (id) {
		url += id + '/';
	}
	return $.ajax({
		url: url,
		beforeSend: function (xhr) { setAuthHeader(xhr); },
		success: function(xml) {
			console.log("Loaded subform data");
			displaySubformData(view, id, [null, xml], tab);
		},
		error: function(xml) {
			console.log('Error loading subform data');
			displaySubformData(view, id, xml, tab); /* needed for events */
		}
	});
}

/*****************************************************************************/
function addSubformEvent(object, view, parentid, tab) {
	/* attach click event to add rows to subform */
	console.log('addSubformEvent()');
	console.log('Adding row to subform parent ' + parentid);

	console.log('object:' + object);
	console.log('view:' + view);
	console.log('parentid:' + parentid);

	/* the part before the underscore is the parent collection */
	var parent_collection = view.split('_')[0];
	var subform_collection = view.split('_')[1];

	var url = collection_url(subform_collection);
	var xml = createRequestXml();

	/* open xml element for the subform object */
	xml += '<' + subform_collection.slice(0,-1) + '>';

	/* find inputs */
	var row = object.parent().parent();
	row.find('input').each(function() {
		var input_name = $(this).attr('name');
		if (input_name) {
			if (input_name == subform_collection) {
				xml += '<' + input_name + ' id="';
				xml += escapeHTML($(this).val());
				xml += '"/>';
			}
			else {
				xml += '<' + input_name + '>';
				xml += escapeHTML($(this).val());
				xml += '</' + input_name + '>';
			}
		}
	});

	/* deal with select(s) */
	row.find('select').each(function() {
		console.log('deal with select(s)');
		var input_name = $(this).attr('name');
		if (input_name == 'relationship') {
			xml += '<relationship organisation="'
			xml += parentid + '" type="0" />';
		}
		if (input_name) {
			console.log('I have <select> with name: ' + input_name);

			$(this).find('option:selected').each(function() {
				xml += '<' + input_name;
				xml += ' ' + parent_collection + '="' + parentid + '"';
				xml += ' type="' + escapeHTML($(this).val())
				xml += '"/>';
			});
		}
	});

	/* close subform object element */
	xml += '</' + subform_collection.slice(0,-1) + '>';

	xml += '</data></request>';

	$.ajax({
		url: url,
		type: 'POST',
		data: xml,
		contentType: 'text/xml',
		beforeSend: function (xhr) { setAuthHeader(xhr); },
		success: function(xml) {
			console.log('SUCCESS: added row to subform');
			loadSubformData(view, parentid, tab);
		},
		error: function(xml) {
			console.log('ERROR adding row to subform');
		}
	});
}

/*****************************************************************************/
/* return a tr with odd or even class as appropriate */
function newRow(i) {
	if (i % 2 == 0) {
		return row = $('<tr class="even">');
	} else {
		return row = $('<tr class="odd">');
	}
}

/*****************************************************************************/
function markComboSelections(combo, typedata) {
	if (typedata) {
		var types = typedata.split(',');
		if (types.length > 0) {
			for (var j=0; j < types.length; j++) {
				var opt = 'option[value=' + types[j] +']';
				combo.find(opt).attr('selected', true);
			}
		}
	}
}

/*****************************************************************************/
function clearForm(datatable) {
	datatable.find('input[type=text]').each(function() {
		$(this).val('');
	});
	datatable.find('input[name=qty]').each(function() {
		$(this).val('1');
	});
	datatable.find('input[type=email]').each(function() {
		$(this).val('');
	});
	datatable.find('select.chzn-done:not(.sub)').val('');
	datatable.find('select.chzn-done:not(.sub)').trigger("liszt:updated");
}

/*****************************************************************************/
/* We've loaded data for a subform; display it */
function displaySubformData(view, parentid, xml, tab) {
	console.log('displaySubformData()');
	var mytab = getTabById(tab);
	var datatable = mytab.find('div.' + view).find('table.datatable');
	var types = [];
	datatable.find('tbody').empty();

	if (xml) {
		if (view == 'salesorderitems') {
			addSalesOrderProducts(tab, datatable, xml[1]);
		}
		else {
			addSubFormRows(xml[1], datatable, view, tab);
		}
	}

	datatable.find('select.chosify').chosen(); /* format combos */
	datatable.find('tbody').fadeIn(300);       /* display table body */

	/* attach click event to remove rows from subform */
	datatable.find('button.removerow').click(function() {
		btnClickRemoveRow(view, parentid, $(this).parent());
	});

    /* "Link Contact" button event handler for organisation form */
    mytab.find('button.linkcontact').click(function() {
		btnClickLinkContact(parentid, tab);
	});

    /* "Apply Tax" button event handler for product form */
    mytab.find('button.taxproduct').click(function() {
		var c = $(this).parent().parent().find('select.tax').val();
		taxProduct(parentid, c, true, tab);
    });
}

/*****************************************************************************/
function btnClickLinkContact(parentid, tab) {
	var mytab = getTabById(tab);
	var c = mytab.find('select.contactlink').val();
	relationshipUpdate(parentid, c, false, true);
}

/*****************************************************************************/
function btnClickRemoveRow(view, parentid, trow) {
	console.log('btnClickRemoveRow(' + view + ',' + parentid + ')');

	if (view == 'salesorderitems') {
		console.log('skipping btnClickRemoveRow() for salesorder');
		return;
	}

	var id = trow.find('input[name="id"]').val();
	var url = collection_url(view) + parentid + '/' + id + '/';
	console.log('DELETE ' + url);
	trow.parent().hide();
	$.ajax({
		url: url,
		type: 'DELETE',
		beforeSend: function (xhr) { setAuthHeader(xhr); },
		complete: function(xml) {
			trow.parent().remove();
		}
	});
}

/*****************************************************************************/
/* build xml and submit form */
function submitForm(object, action, id) {
	var xml = createRequestXml();
	var url = '';
	var collection = '';
	var mytab = activeTab();
	var subid = null;

	/* if object has subforms, which xml tag do we wrap them in? */
	if (object == 'salesorder') {
		var subobject = 'salesorderitem';
	}

	console.log('Submitting form ' + object + ':' + action);

	/* find out where to send this */
	mytab.find(
		'div.' + object
	).find('form:not(.subform)').each(function() {
		collection = $(this).attr('action');
		url = collection_url(collection);
		if (id) {
			url += id;
		}
	});

	console.log('submitting to url: ' + url);

	/* build xml request */
	xml += '<' + object + '>';
	mytab.find(
		'div.' + object
	)
	.find('input:not(.nosubmit,default),select:not(.nosubmit,.default)')
	.each(function() {
		var name = $(this).attr('name');
		if (name) {
            var o = new Object();
            if (customFormFieldHandler($(this), o)) {
                xml += o.xml;
            }
            else {

				if (name == 'subid') {
					subid = $(this).val();
				}
				/* FIXME: move to application */
				console.log('processing input ' + name);
				if ((name != 'id') && (name != 'subid')
				&& ((name != 'relationship')
				||(object == 'organisation_contacts')))
				{
					if ($(this).attr('type') == "checkbox") {
                        xml += '<' + name + '>';
                        xml += $(this).prop('checked') ? '1' : '0';
                        xml += '</' + name + '>';
                    }
                    else {	
						console.log($(this).val());
						if ($(this).hasClass('sub')) {
							/* this is a subform entry, so add extra xml tag */
							xml += '<' + subobject;
							if (subid) {
								xml += ' id="' + subid + '"';
								subid = null;
							}
							xml += '>';
						}
						/* save anything that has changed */
						if ($(this).val() != $(this).data('old')) {
							xml += '<' + name + '>';
							xml += escapeHTML($(this).val());
							xml += '</' + name + '>';
						}
						if ($(this).hasClass('endsub')) {
							/* this is a subform entry, so close extra xml tag */
							xml += '</' + subobject + '>';
						}
					}
				}
			}
		}
	});

	/* TODO: temp - add Standard Rate VAT to products by default */
	/* FIXME */
	if (object == 'product') {
		xml += '<tax id="1"/>';
	}

	xml += '</' + object + '>';
	xml += '</data></request>';

	/* tell user to wait */
	if ((action == 'create') || (action == 'update')) {
		showSpinner('Saving ' + object + '...');
	}
	else {
		showSpinner(); 
	}
	postXML(url, xml, object, action, id, collection);
}

function postXML(url, xml, object, action, id, collection) {
	/* send request */
    $.ajax({
        url: url,
        type: 'POST',
        data: xml,
        contentType: 'text/xml',
		timeout: g_timeout,
		beforeSend: function (xhr) { setAuthHeader(xhr); },
        success: function(xml) {
            hideSpinner();
            TABS.refresh(collection);
            submitFormSuccess(object, action, id, collection, xml);
        },
        error: function(xhr, s, err) {
			console.log(err);
			if (s == 'timeout') {
				console.log('timeout');
				statusMessage('Timeout.  ' + object + ' not ' + action + 'd.',
					STATUS_CRIT);
				hideSpinner();
			}
			else {
				submitFormError(object, action, id);
			}
		}
    });
}

/* To be overridden by application */
function customFormFieldHandler(input, o) {
    return false;
}

/*****************************************************************************/
/* Replace HTML Special Characters */
function escapeHTML(html) {
	var x;
	var hchars = {
		'&': '&amp;',
		'<': '&lt;',
		'>': '&gt;',
		'"': '&quot;'
	};

	for (x in hchars) {
		html = html.replace(new RegExp(x, 'g'), hchars[x]);
	}

	return html;
}

/*****************************************************************************/
function submitFormSuccess(object, action, id, collection, xml) {
    if (!customSubmitFormSuccess(object, action, id, collection, xml)) {
        return;
    }

	/* check for tabs that will need refreshing */
	tabRefresh(collection);
	
	/* We received some data back. Display it. */
	newid = $(xml).find(object).text();
	if (newid) {
		console.log('data returned from POST');
		if (action == 'create') {
			console.log('id of ' + object + ' created was ' + newid);
			action = 'update';
			var d = new Array();
			var formURL = '/html/forms/' + object + '/' + action + '.html';
			d.push(getHTML(formURL));   /* fetch html form */
			$.when.apply(null, d)
			.done(function(html) {
				displayForm(object, action, null, html, [xml], activeTabId());
			})
			.fail(function() {
				console.log('failed to fetch html form');
				hideSpinner();
			});
		}
	}

	if (action == 'create') {
		/* clear form ready for more data entry */
		console.log('refreshing data entry form');
		//getForm(object, action, null, null, activeTabId());
		getForm(object, action, TABS.active.title);
	}

	hideSpinner();
}

/*****************************************************************************/
function submitFormError(object, action, id) {
	hideSpinner();
	if ((object == 'salesorder' && action == 'process')) {
		statusMessage('Billing run failed', STATUS_CRIT);
	}
	else {
		statusMessage('Error saving ' + object, STATUS_CRIT);
	}
}

/*****************************************************************************/
/* return the singular object name for a given collection */
function collectionObject(c) {
	if (c == 'salesinvoices') {
		return 'salesinvoice';
	}
	else if (c.substring(c.length - 2) == 'es') {
		/* plural ending in 'es' */
		return c.substring(0, c.length - 2);
	}
	else {
		/* the general case
		 * - lop off the last character and hope for the best */
		return c.substring(0, c.length - 1);
	}
}

/*****************************************************************************/
/* Fetch an individual element of a collection for display / editing */
function displayElement(collection, id, title) {
	console.log('displayElement(' + collection + ',' + id + ',' + title + ')');
	var object = collectionObject(collection);
	if (!title) {
		var title = 'Edit ' + object.substring(0,1).toUpperCase()
		+ object.substring(1) + ' ' + id;
	}
	if (id) {
		var action = 'update';
	}
	else {
		var action = 'create';
	}

	if (collection.substring(0, 8) == 'reports/') {
		showHTML(collection_url(collection) + id, title, false);
		return;
	}

	showSpinner(); /* tell user to wait */

	/* fetch the xml and html we need, then display the form */
	var d = fetchElementData(collection, id, object, action);
	$.when.apply(null, d)
	.done(function(html) {
		/* all data is in, display form */
		var args = Array.prototype.splice.call(arguments, 1);
		displayForm(object, action, title, html[0], args);
	})
	.fail(function() {
		/* something went wrong */
		statusMessage('error loading data', STATUS_CRIT);
		hideSpinner();
	});
}

/*****************************************************************************/
function fetchElementData(collection, id, object, action) {
	console.log('fetchElementData(' + collection + ',' + id + ')');
	var dataURL = collection_url(collection) + id;
	var formURL = '/html/forms/' + object + '/' + action + '.html';
	var d = new Array(); /* array of deferreds */

	/* stack up our async calls */
	d.push(getHTML(formURL));	/* fetch html form */

	if (id) {
		d.push(getXML(dataURL));	/* fetch element data */
	}

	/* fetch any other data we need:
	 *  first, any subform rows, 
	 *  then, combos in the order they appear on the form */
	if (collection == 'accounts') {
		d.push(getXML(collection_url('accounttypes')));
	}
	else if (collection == 'organisations') {
		d.push(getXML(collection_url('organisation_contacts') + id + '/'));
		d.push(getXML(collection_url('relationships')));
		d.push(getXML(collection_url('contacts')));
	}
	else if (collection == 'products') {
		d.push(getXML(collection_url('product_taxes') + id + '/'));
		d.push(getXML(collection_url('accounts')));
		d.push(getXML(collection_url('taxes')));
	}
	else if (collection == 'salesorders') {
		d.push(getXML(collection_url('salesorderitems') + id + '/'));
		d.push(getXML(collection_url('cycles')));
		d.push(getXML(collection_url('products')));
	}

	return d;
}

/*****************************************************************************/
function getHTML(url) {
	return $.ajax({
		url: url,
		type: 'GET',
		dataType: 'html',
		beforeSend: function (xhr) { setAuthHeader(xhr); }
	});
}
/*****************************************************************************/
function getXML(url, async) {
	async = async === undefined ? true : false; /* default to true */
	return $.ajax({
		url: url,
		type: 'GET',
		async: async,
		dataType: 'xml',
		timeout: g_timeout,
		beforeSend: function (xhr) { setAuthHeader(xhr); },
		fail: function() {
			console.log('getXML(' + url + ') failed.  Retrying');
			getXML(url, async);
		},
	});
}

/* build a table structure from xml using divs */
function divTable(div, xml) {
	div.empty();

    if ($(xml).find('resources').children().length == 0) {
        console.log('No results');
        return false; /* no results */
    }   

    /* build a table structure using divs */
    var formtable = $('<div class="formtable"/>');
    div.append(formtable);
    var row = 0;
    $(xml).find('resources').find('row').each(function() {
        row++;
        tr = $('<div class="tr"/>');
        formtable.append(tr);
		$(this).children().each(function() {
			var n = 'xml-' + this.tagName;
			var v = ($(this).text() != '') ? $(this).text() : '&nbsp;';
			var td = $('<div class="td ' + n + '">' + v + '</div>');
			tr.append(td);
		});
    });
	//var clearfix = $('<div class="clearfix"/>');
	//formtable.append(clearfix);
}

function oddEven(row) {
	return ((row % 2) == 0) ? 'even' : 'odd';
}

function toggleSelected(o) {
    if (o.hasClass('selected')) {
        o.removeClass('selected');
    }
    else {
        o.addClass('selected');
    }
}

function deselectAllRows(o) {
	o.parent().find('div.tr').removeClass('selected');
}

function selectAllRows(o) {
	o.parent().find('div.tr').addClass('selected');
}

function selectRowSingular(o) {
	deselectAllRows(o);
	toggleSelected(o);
}

/* build a basic accordion object */
function accordionize(o) {
	o.attr('height');
	o.find('div > h3').each(function() {
		$(this).click(accordionClick);
	});
}

function accordionClick() {
	if ($(this).next().visible) return;
	$(this).parents('div.accordion').find('div.content').hide();
	$(this).parents('div.accordion').find('div.section > h3')
		.removeClass('selected');
	$(this).addClass('selected');
	$(this).next().fadeIn(250);
}

$.fn.accordionTab = function(n) {
    return $(this).find('div.accordion div.section:nth-child(' + n + ') div');
}

$.fn.accordionTitle = function(n) {
    return $(this).find('div.accordion div.section:nth-child(' + n + ') h3');
}

$.fn.accordionTabSelect = function(n) {
    $(this).accordionTitle(n).each(accordionClick);
}


/*****************************************************************************/
/* display XML results as a sortable table */
function displayResultsGeneric(xml, collection, title, sorted, tab, headers) {
	var refresh = false;

	statusHide();

	if ([
		'accounts',
		'contacts',
		'departments',
		'divisions',
		'organisations',
		'products',
	].indexOf(collection) != -1) 
	{
		refresh = true;
	}
	
	if ($(xml).find('resources').children().length == 0) {
		/* No results found */
		hideSpinner();
		if (collection == 'contacts') {
			getForm('contact', 'create', 'Add New Contact');
		}
		else if (collection == 'organisations') {
			getForm('organisation', 'create', 'Add New Organisation');
		}
		else if (collection == 'products') {
			getForm('product', 'create', 'Add New Product');
		}
		else {
			$t = '<div class="results empty">Nothing found</div>';
		    if (tab) {
				updateTab(tab, $t);
			}
			else {
				var tab = new Tab(title, $t, true, collection, refresh);
			}
		}
		return;
	}

	$t = '<a class="source" href="' + collection_url(collection) + '"/>';
	$t += '<table class="datatable ' + collection + '">';
	$t += "<thead>";
	$t += "<tr>";
	var row = 0;
	$(xml).find('resources').find('row').each(function() {
		row++;
		if (row == 1) {
			$(this).children().each(function() {
				$t += '<th class="xml-';
				if (headers) {
					/* use first row as headers */
					$t += $(this).text().toLowerCase() + '">';
					$t += $(this).text();
				}
				else {
					$t += this.tagName + '">';
					$t += this.tagName;
				}
				$t += '  </th>';
			});
			$t += "</tr>";
			$t += "</thead>";
			$t += "<tbody>";
		}
		if (row % 2 == 0) {
			$t += '<tr class="even ' + this.tagName  + '">';
		} else {
			$t += '<tr class="odd ' + this.tagName  + '">';
		}
		$(this).children().each(function() {
			if (!((row == 1) && (headers))) {
				$t += '<td class="xml-' + this.tagName + '">'
				if ((this.tagName == 'price_buy')
				|| (this.tagName == 'price_sell'))
				{
					$t += decimalPad($(this).text(), 2);
				}
				else if (this.tagName == 'nominalcode'){
					$t += padString($(this).text(), 4);
				}
				else {
					$t += $(this).text();
				}
				/* add trailing space if numeric value and positive */
				if ((this.tagName == 'debit') || (this.tagName == 'credit') 
				 || (this.tagName == 'total') || (this.tagName == 'amount')
				 || (this.tagName == 'tax') || (this.tagName == 'subtotal'))
				{
					if ($(this).text().substr(-1) != ')') {
						$t += ' ';
					}
				}
				$t += '</td>';
			}
		});
		/* quick hack to add pdf link */
		if (collection == 'salesinvoices') {
			var ref = $(this).find('ref').text();
			var si = 'SI-' + ref.replace('/','-') + '.pdf';
			var siref = 'SI/' + ref;
			$t += '<td class="xml-pdf">';
			$t += '<a id="' + siref + '" href="/pdf/' + g_orgcode + '/';
			$t += si + '">[PDF]</a>';
			$t += '</td>';
		}
		$t += "</tr>";
	});
	$t += "</tbody>";
	$t += "</table>";

	if (! title) {
		title = "Results";
	}

	$t = $($t); /* htmlify */

	/* attach click event */
	$t.find('tr').click(clickElement);

	if (tab) {
		/* refreshing existing tab */
		updateTab(tab, $t);
	}
	else {
		addTab(title, $t, true, collection, refresh);
	}

	/* make our table pretty and sortable */
	if (sorted) {
		getTabById(tab).find(".datatable").tablesorter({
			sortList: [[0,0], [1,0]], 
			widgets: ['zebra'] 
		});
	}

	hideSpinner();
}

function clickElement() {
	var row = $(this);
	var tab = TABS.active;
	if (!customClickElement(row)) {
		var id = row.find('td.xml-id').text();
		displayElement(tab.collection, id);
	}
}

/* to be overridden by application */
function customClickElement(row) {
	return false;
}

/*****************************************************************************/
/* hide please wait dialog */
function showSpinner(message) {
	if (message) {
		$('.spinwaittxt').text(message);
	}
	else {
		$('.spinwaittxt').text('Please wait...');
	}
	$("#loading-div-background").show();
}

/*****************************************************************************/
/* hide please wait dialog */
function hideSpinner() {
	$("#loading-div-background").hide();
}

/*****************************************************************************/
/* return url for collection */
function collection_url(collection) {
	var url;
	url =  '/' + g_instance + '/' + g_business + '/' + collection;
    if (collection.substr(collection.length-1, 1) !== '/') url += '/';
	return url;
}

/*****************************************************************************/
/* Javascript has no decimal type, so we need to teach it how to add up */
function decimalAdd(x, y) {
	x = new Big(x);
	y = new Big(y);
	return x.plus(y);
}

/*****************************************************************************/
/* Return x-y */
function decimalSubtract(x, y) {
	x = new Big(x);
	y = new Big(y);
	return x.minus(y);
}

/*****************************************************************************/
/* Compare two string representations of decimals numerically */
function decimalEqual(term1, term2) {
	return (Number(term1) == Number(term2));
}

/*****************************************************************************/
/* Return string representation of decimal padded to at least <digits> 
 * decimal places */
function decimalPad(decimal, digits) {
	/* if decimal is blank, or NaN, make it zero */
	if ((isNaN(decimal)) || (decimal == '')) {
		decimal = '0';
	}

	/* first, convert to a number and back to a string - this strips any 
	 * useless leading and trailing zeros. We'll put back the ones we need. */
	decimal = String(Number(decimal));

	var point = String(decimal).match(/\./g);

	if (point) {
		pindex = String(decimal).indexOf('.');
		places = String(decimal).length - pindex - 1;
		if (places < digits) {
			decimal = String(decimal) + Array(digits).join('0');
		}
	}
	else if (digits == 0) {
		decimal = String(decimal);
	}
	else if (digits > 0) {
		decimal = String(decimal) + '.' + Array(digits + 1).join('0');
	}
	return decimal;
}

/*****************************************************************************/
/* pad out a string with leading zeros */
function padString(str, max) {
	str = '' + str;
	return str.length < max ? padString("0" + str, max) : str;
}

/*****************************************************************************/
function roundHalfEven(n, dp) {
	var x = Big(n);
	return x.round(dp, 2);
}

/*****************************************************************************/
/* Start building an xml request */
function createRequestXml() {
	var xml = '<?xml version="1.0" encoding="UTF-8"?><request>';
	xml += '<instance>' + g_instance + '</instance>';
	xml += '<business>' + g_business + '</business>';
	xml += '<data>';
	return xml;
}

/*****************************************************************************/
/* create business selector combo */
function prepBusinessSelector() {
	$.ajax({
		url: collection_url('businesses'),
		beforeSend: function (xhr) { setAuthHeader(xhr); },
		success: function(xml) {
			showBusinessSelector(xml);
		},
		error: function(xml) {
			customBusinessNotFound(xml);
		}
	});
}

/* to be overridden by application */
function customBusinessNotFound(xml) {
}

/*****************************************************************************/
/* Display combo for switching between businesses */
function showBusinessSelector(xml) {
	if ($(xml).find('row').length == 0) {
		/* No businesses found */
		getForm('business', 'create', 'Add New Business');
		return;
	}

	select = $('select.businessselect');
	select.empty();

	g_xml_business = xml;

	$(xml).find('row').each(function() {
		var id = $(this).find('id').text();
		var name = $(this).find('name').text();
		select.append($("<option />").val(id).text(name));
	});
	
	select.change(function() {
		switchBusiness($(this).val());
	});

	$('select.businessselect').val(g_business);
	switchBusiness(g_business);
}

/*****************************************************************************/
/* Switch to the selected business */
function switchBusiness(business) {
	/* hide content of active tab */
	$('.tablet.active').addClass('hidden');

	/* hide all tabheads for this business */
	$('.tabhead.business' + g_business).each(function() {
		$(this).addClass('hidden');
	});

	/* switch business */
	g_business = business;

	/* find orgcode for this business */
	$(g_xml_business).find('row').each(function() {
		if ($(this).find('id').text() == business) {
			g_orgcode = $(this).find('orgcode').text();
		}
	});

	/* unhide tabs for new business */
	$('.tabhead.business' + g_business).each(function() {
		$(this).removeClass('hidden');
	});
	$('.tablet.business' + g_business).each(function() {
		$(this).removeClass('hidden');
	});
}

/*****************************************************************************/
function mapUpdate() {
	console.log('mapUpdate()');
	var tab = activeTab();
	var addressFields = ['line_1','line_2','line_3','town','county',
            'country','postcode'];
	var geodata = new Array();
	var selector = addressFields.join('"],input[name="'); 
	selector = 'input[name="' + selector + '"]';
	mytab.find(selector).each(function() {
		if ($(this).val().length > 0) { geodata.push($(this).val()); }
	});

	/* load map */
	if (geodata.length > 0) {
		var locString = geodata.join();
		console.log('locString:' + locString);
		loadMap(locString, mytab);
	}
}

function loadMap(locationString, tab) {
	var canvas;
	var map;

	var mapOptions = {
	    zoom: 15,
		mapTypeId: google.maps.MapTypeId.ROADMAP
	}

	if (tab) {
		mytab = getTabById(tab);
	}
	else {
		mytab = activeTab();
	}

	mytab.find('.map-canvas').each(function() {
		console.log('found a map-canvas');
		canvas = this;
		$(canvas).empty();
		map = new google.maps.Map(canvas, mapOptions);
		$(canvas).fadeIn(300, function() {
			renderMap(map, locationString);
		});
	});


}

function renderMap(map, locationString) {
	var geocoder = new google.maps.Geocoder();
	geocoder.geocode( { 'address': locationString}, function(results, status) {
		if (status == google.maps.GeocoderStatus.OK) {
			map.setCenter(results[0].geometry.location);
			var marker = new google.maps.Marker({
				map: map,
				position: results[0].geometry.location
			});
		} else {
			console.log("Geocode was not successful for the following reason: "
				+ status);
		}
	});
	google.maps.event.trigger(map, "resize");
}

/* create tab object */
function newtab() {
	var tab = new Object();
	tab.id = ++g_tabid;
	tab.classes = 'tabhead tablet' + tab.id + ' business' + g_business;
	tab.contentClasses = 'tablet tablet' + tab.id + ' business' + g_business;
	return tab;
}

function randomString(strlen) {
    var a = new Array();
    var set = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

    for(var i=0; i < strlen; i++) {
        a.push(set.charAt(Math.floor(Math.random() * set.length)));
    }
    return a.join('');
}

function isDate(datestring) {
	var dte = new Date(datestring);
	return (!isNaN(dte) && datestring.length == 10);
}

/* setTimeout hacks 
  source: https://developer.mozilla.org/en-US/docs/Web/API/Window.setTimeout */
var __nativeST__ = window.setTimeout, __nativeSI__ = window.setInterval;
 
window.setTimeout = function (vCallback, nDelay /*, argumentToPass1, argumentToPass2, etc. */) {
  var oThis = this, aArgs = Array.prototype.slice.call(arguments, 2);
  return __nativeST__(vCallback instanceof Function ? function () {
    vCallback.apply(oThis, aArgs);
  } : vCallback, nDelay);
};
 
window.setInterval = function (vCallback, nDelay /*, argumentToPass1, argumentToPass2, etc. */) {
  var oThis = this, aArgs = Array.prototype.slice.call(arguments, 2);
  return __nativeSI__(vCallback instanceof Function ? function () {
    vCallback.apply(oThis, aArgs);
  } : vCallback, nDelay);
};

/* selector for elements that do not have the specified parents
 * eg. $('div.blerk:notparents(div.blah)'); */
jQuery.expr[':'].notparents = function(a, i, m) {
    return jQuery(a).parents(m[3]).length === 0;
};

/* trim() */
if(typeof(String.prototype.trim) === "undefined")
{
    String.prototype.trim = function() {
		return String(this).replace(/^\s+|\s+$/g, '');
	};
}

/* currency format 
 * (bankers rounding => 2 decimal places, comma-separate '000s */
if (typeof (String.prototype.formatCurrency) === "undefined") {
    String.prototype.formatCurrency = function() {
        var tmp = String(this);
        tmp = roundHalfEven(this, 2);
        tmp = decimalPad(tmp, 2);
        return formatThousands(tmp);
    };
}

function form_url(form) {
	return g_url_form + form.object + '/' + form.action + '.html';
}

/* Create a Form, populate and display it in a Tab */
function showForm(object, action, title, id) {
    console.log('showForm(' + object + '.' + action + ')');
	showSpinner();
	var form = new Form(object, action, title, id);
	form.load()
	.done(function() {
		form.activate();
		hideSpinner();
	})
	.fail(function () {
		hideSpinner();
	});
	return form.tab;
}

/* Form() */
function Form(object, action, title, id) {
	if (!(this instanceof Form))
		return new Form(object, action, title);
	console.log('Form() constructor');
	this.collection = object + 's';
	this.object = object;
	this.action = action;
	this.id = id;
	this.title = title;
	this.data = {};
	this.sources = this.dataSources();
}

/* Activate this Form's Tab, .show()ing it first if needed */
Form.prototype.activate = function() {
	if (this.tab == undefined) this.show();
	console.log('Form().activate()');
	this.tab.activate();
	return this;
}

/* to be overridden by application */
Form.prototype.customXML = function() {
	/* append any extra bits to this.xml */
}

Form.prototype.dataSources = function() {
    var sources;
    if (FORMDATA !== undefined) {
        if (FORMDATA[this.object] !== undefined) {
            sources = FORMDATA[this.object][this.action].slice();
        }
    }
    return sources;
}

/* Set up Form events */
Form.prototype.events = function() {
	console.log('Form().events()');
	var form = this;
	var t = this.tab.tablet;
	t.find('a.tablink').off().click(function() {
        clickTabLink()
    });
	t.find('button.add').off().click(function() {
        var subform = $(this).closest('div.form'); /* form parent */
        form.rowAdd(subform);
		return false;
	});
	t.find('button.cancel').off().click(function() {
		form.tab.close();
		return false;
	});
	t.find('button.del').off().click(function() {
        form.rowDelete($(this));
		return false;
	});
	t.find('button.reset').off().click(function(event) {
        event.preventDefault();
        form.reset();
		return false;
	});
	t.find('button.save').off().click(function() {
		if (form.validate()) form.submit();
		return false;
	});
	t.find('.onchange').off().change(function() {
        form.onChange($(this));
        return false;
    });
    customBlurEvents(this.tab.id);
}

/* load some data */
Form.prototype.fetchData = function() {
	console.log('Form().fetchData()');
	var d = new Array(); /* array of deferreds */
	d.push(getHTML(form_url(this)));
	if (this.action !== 'create' && this['FORMDATA'] === undefined) {
		d.push(getXML(collection_url(this.collection) + this.id));
	}
    if (this.sources !== undefined) {
        for (var i=0, l=this.sources.length; i < l; i++) {
            var collection = this.sources[i].replace('{id}', this.id);
            if (this.sources[i] !== collection) this.sources[i] = collection;
            var url = collection_url(collection);
            d.push(getXML(url));
        }
    }
	console.log('loading 1 html + ' + l + ' xml documents');
	return d;
}

/* put finishing touches on form */
Form.prototype.finalize = function() {
	console.log('Form().finalize()');
    this.formatDatePickers();                   /* date pickers */ 
    this.formatRadioButtons();                  /* tune the radio */
	this.events();
    this.updateMap();
    this.updateAllTotals();
}

Form.prototype.formatDatePickers = function() {
    console.log('Form().formatDatePickers()');
    formatDatePickers(this.tab.id);
}

Form.prototype.formatRadioButtons = function() {
    console.log('Form().formatRadioButtons()');
    formatRadioButtons(this.tab.id, this.object);
}

/* fill form row based on data source */
Form.prototype.formFill = function(ctl) {
    console.log('Form().formFill()');
    var row = ctl.closest('div.tr');

    /* if control is select and nothing selected */
    if (ctl.is('select')) {
        if (ctl.val() === ctl.find('option:first').val()) {
            /* nothing selected, reset placeholders */
            row.find('input').each(function() {
                if ($(this).data('placeholder.orig') !== undefined) {
                    $(this).attr('placeholder',
                        $(this).data('placeholder.orig'));
                    $(this).trigger('change');
                }
            });
            return this;
        }
    }

    var d = $(this.data[ctl.data('source')]);
    /* find matching row in data source */
    var r = d.find('resources row').filter(function() {
        return $(this).find('id').text() === ctl.val();
    });
    /* fill row from datasource */
    r.children().each(function() {
        var fld = row.find('[name="' + this.tagName + '"]');
        if (fld !== undefined) {
            if (fld.data('placeholder.orig') === undefined) {
                fld.data('placeholder.orig', fld.attr('placeholder'));
            }
            var newval = $(this).text();
            if (fld.hasClass('currency')) newval = decimalPad(newval, 2);
            fld.attr('placeholder', newval);
            /* field blank, so placeholder change triggers change event */
            if (fld.val() === '') fld.trigger('change');
        }
    });
}

/* Does the tab for this form have a map? */
Form.prototype.hasMap = function() {
    return (this.map !== undefined);
}

/* Load HTML form + XML data sources.  Return deferred. */
Form.prototype.load = function() {
	console.log('Form().load()');
	var form = this;
	var object = this.object;
	var action = this.action;
	var d = this.fetchData();
	return $.when.apply(null, d)
	.done(function(html) {
		console.log('Form() data loaded');
		form.html = (Array.isArray(html)) ? html[0] : html;
		var data = Array.prototype.splice.call(arguments, 1);
		if (action == 'create') {
			form.data['FORMDATA'] = null;
		}
		else if (form.data['FORMDATA'] === undefined) {
			form.data['FORMDATA'] = $(data.shift()[0]);
        }
		else {
			data.shift();
		}
		form.updateDataSources(data);
		console.log('Form(' + object + '.' + action + ').load() done');
	})
	.fail(function() {
		console.log('Form(' + object + '.' + action + ').load() fail');
	});
	return this;
}

/* handle form controls with onchange events */
Form.prototype.onChange = function(ctl) {
    console.log('Form().onChange()');
    if (ctl.hasClass('formfill')) this.formFill(ctl);
    if (ctl.hasClass('addend')) this.rowSum(ctl);
    if (ctl.hasClass('multiplicand')) this.rowMultiply(ctl);
    if (ctl.hasClass('currency') && ctl.val() !== '' && !isNaN(ctl.val())) {
        ctl.val(decimalPad(roundHalfEven(ctl.val(), 2), 2));
    }
    if (ctl.hasClass('sum')) this.updateFormTotals(ctl);
}

/* Fill html form with data */
Form.prototype.populate = function() {
	console.log('Form().populate()');
	if (this.data.length == 0) {
		console.log('No data to populate form.');
	}
	var id = undefined;
	var form = this;
	this.workspace = $(this.html); /* start with base html */
    /* set up Map */
	if (this.workspace.filter('div.map-canvas').length !== 0) {
        this.map = new Map();
    }
	this.collection = this.workspace.find('form:not(.subform)').first()
		.attr('action');
	var w = this.workspace.filter('div.' + this.object + '.' + this.action)
		.first();
	/* populate combos */
	w.find('select.populate:not(.sub)').each(function() {
		var xml = form.data[$(this).attr('data-source')];
		$(this).populate(xml);
	});
	/* populate form fields using first xml doc */
	if (this.data["FORMDATA"]) {
		this.data["FORMDATA"].find('resources row').first().children()
		.each(function()
		{
			var tagName = this.tagName;
			var tagValue = $(this).text();
			console.log(tagName + '=' + tagValue);
			if (tagName == 'id' || tagName == this.object) {
				form.id = tagValue; /* grab id */
			}
			var fld = w.find('[name="' + tagName + '"]');
			if (fld.length > 0) {
				fld.val(tagValue); /* set field value */
				fld.data('old', tagValue);/* note the unmodifed value */
				if (form.hasMap()) { /* store map geo data */
					if (MAPFIELDS[form.object].indexOf(tagName) != -1
					&& tagValue.length > 0)
					{
						form.map.addGeo(tagValue);
					}
				}
			}
			else {
				console.log('field "' + tagName + '" not found on form');
			}
		});
	}
	this._populateSubforms();

	/* where do we POST this form? */
	this.url = collection_url(this.collection);
	if (this.id) this.url += this.id;
	console.log('Form().url=' + this.url);
}

Form.prototype._populateSubforms = function() {
    console.log('Form()._populateSubforms()');
	var form = this;
    var subform = this.workspace.find('form.subform');
    var i, d, tag;
    if (subform.length === 0) {
        subform = this.workspace.find('form div.form');
        subform.each(function() {
            var source = $(this).data('tag') + 's/' + form.id + '/';
            var data = $(form.data[source]);
            var datarows = data.find('row');
            var subrows = subform.find('div.subformwrapper div.tr');
            for (i = subrows.length; i < datarows.length; i++) {
                form.rowAdd(subform);
            }
            subrows = subform.find('div.subformwrapper div.tr');
            $.each(datarows, function(i, d) {
                var id = $(d).find('id');
                if (id.length === 1) subrows.eq(i).data('id', id.text());
                $(d).children().each(function(k, tag) {
                    var tagName = tag.tagName;
                    var tagValue = $(tag).text();
                    var ctl = subrows.eq(i).find('[name="'+ tagName +'"]');
                    ctl.val(tagValue);
                    if (ctl.hasClass('formfill')) form.formFill(ctl);
                });
            });
        });
    }
    else {
        /* deprecated - used by organisations form */
        subform.each(function() {
            var view = $(this).attr('action');
            loadSubformData(view, form.id, form.tab.id);
        });
    }
}

/* POST data */
Form.prototype.post = function() {
	var form = this;
	return $.ajax({
        url: this.url,
        type: 'POST',
        data: this.xml,
        contentType: 'text/xml',
        timeout: g_timeout,
        beforeSend: function (xhr) { setAuthHeader(xhr); },
        success: function(xml) {
			form.submitSuccess(xml);
		},
        error: function(xhr, s, err) {
			form.submitError(xhr, s, err);
        }
    });
}

Form.prototype.reset = function() {
    console.log('Form().reset()');
    var t = this.tab.tablet;
    var f = t.find('form');
    f.get(0).reset();
    /* re-populate form */
    f.find('input,select').each(function() {
        if (($(this).val() !== $(this).data('old'))
        && ($(this).data('old') !== undefined))
        {
            $(this).val($(this).data('old'));
        }
        /* reset placeholder */
        if ($(this).data('placeholder.orig') !== undefined) {
            $(this).attr('placeholder', $(this).data('placeholder.orig'));
            $(this).trigger('change');
        }
    });
    /* reset subforms */
    f.find('div.form div.tr.sub:not(:first)').remove();
    this._populateSubforms();
    this.updateAllTotals();
    statusHide(); /* clear status Message */
}

/* add row to subform */
Form.prototype.rowAdd = function(subform) {
    console.log('Form().rowAdd()');
    var object = subform.data('object');
    var row = subform.find('div.subformwrapper div.tr:first').clone(true,true);

    /* reset values of cloned row */
    row.removeData();
    row.find('input').each(function() {
        var d = $(this).data('default');
        if (d !== undefined) {
            $(this).val(d);
        }
        else {
            $(this).val('');
        }
        /* reset placeholder */
        if ($(this).data('placeholder.orig') !== undefined) {
            $(this).attr('placeholder', $(this).data('placeholder.orig'));
        }
    });
    row.find('select').each(function() { 
        $(this).val($(this).find('option:first').val());
    });

    var wrapper = subform.find('div.subformwrapper');
    if (wrapper.length !== 0) {
        /* subform has subformwrapper, so append in here */
        wrapper.append(row);
    }
    else {
        subform.append(row);
    }
}

/* Delete a row.  Actually, we hide it and mark it deleted so it is deleted
 * when we save. */
Form.prototype.rowDelete = function(ctl) {
    console.log('Form().rowDelete()');
    var row = ctl.closest('div.tr')
    var id = row.data('id');
    if (id === undefined) { /* row has never been saved, just remove it */
        row.remove();
    }
    else {                  /* row needs to be marked deleted on the server */
        row.hide();
        var td = row.find('div.td');
        td.children(':not(.stet)').remove();
        td.append('<input type="hidden"/>'); /* placeholder for record */
        row.data('deleted', 'true');
    }
    this.updateAllTotals();
}

/* muliply each .multiplicand for this row, placing the result in .sum */
Form.prototype.rowMultiply = function(ctl) {
    console.log('Form().rowMultiply()');
    var row = ctl.closest('div.tr');

    /* start with 1, and multiply by each .multiplicand to find .sum */
    row.find('.sum').each(function() {
        $(this).val('1.00');
        var sum = $(this);
        row.find('.multiplicand').each(function() {
            var p = Big(sum.val());
            var q = Big(0);
            if ($(this).val() === '') {
                /* val is blank, use placeholder if numeric */
                if ($(this).attr('placeholder') !== undefined 
                && !isNaN($(this).attr('placeholder'))) 
                {
                    q = Big($(this).attr('placeholder'));
                }
            }
            else if (!isNaN($(this).val())) {
                q = Big($(this).val());
            }
            var t = p.times(q);
            if (sum.hasClass('currency')) {
                t = decimalPad(roundHalfEven(t, 2), 2);
            }
            sum.val(t);
            sum.trigger('change');
        });
    });
}

/* sum each .addend for this row, placing the result in .sum */
Form.prototype.rowSum = function(ctl) {
    console.log('Form().rowSum()');
    var row = ctl.closest('div.tr');

    /* for controls with class "sum", update with the sum of all "addends"*/
    row.find('.sum').each(function() {
        $(this).val('0.00');
        var sum = $(this);
        row.find('.addend').each(function() {
            if ($(this).val() === '') {
                /* val is blank, use placeholder if numeric */
                if ($(this).attr('placeholder') !== undefined 
                && !isNaN($(this).attr('placeholder'))) 
                {
                    sum.val(decimalAdd(sum.val(),$(this).attr('placeholder')));
                }
            }
            else if (!isNaN($(this).val())) {
                sum.val(decimalAdd(sum.val(), $(this).val()));
            }
            sum.trigger('change');
        });
    });
}

/* Create/Update tab with Form content */
Form.prototype.show = function(tab) {
	console.log('Form().show()');
	if (tab === undefined && this.tab === undefined) {
		tab = new Tab(this.title);
		this.tab = tab; tab.form = this; /* link form and tab to each other */
	}
	if (this.title !== undefined) {
		console.log('setting tab title');
		this.tab.setTitle(this.title);
	}
	if (this.html !== undefined) {
		console.log('setting tab content');
		this.populate();
		this.tab.setContent(this.workspace);
		this.finalize();
	}
	else {
		console.log('no tab content');
	}
	return this;
}

/* submit form */
Form.prototype.submit = function() {
	console.log('Form().submit() => ' + this.url);
	var xml = createRequestXml();

	xml += '<' + this.object + '>';
    var tag;
    var form = this.tab.tablet.find('div.' + this.object + '.' + this.action 
        + ' form');
    var inputs = form.find('div.td').children();
    if (inputs.length === 0) inputs = form.find('input,select');
    inputs.each(function() {
		var name = $(this).attr('name');
        /* process everything except checkboxes */
        if ($(this).attr('type') !== "checkbox") {
            /* not a checkbox */
            if (name) console.log(name + '=' + $(this).val());
            var subform = $(this).closest('div.form');

            /* open subform tag? */
            if (subform.length === 1) {
                if ($(this).closest('div.td').is(':first-child')
                && $(this).is(':first-child')) 
                {
                    /* use data-tag if available, otherwise data-object */
                    if (subform.data('tag') !== undefined) {
                        tag = subform.data('tag');
                    }
                    else {
                        tag = subform.data('object');
                    }
                    /* tack in attributes */
                    var attrs = '';
                    var id = $(this).closest('div.tr').data('id');
                    var del = $(this).closest('div.tr').data('deleted');
                    if (id !== undefined) attrs += ' id="' + id + '"';
                    if (del !== undefined) attrs += ' is_deleted="true"';
                    xml += '<' + tag + attrs + '>';
                }
            }

            /* save anything that has changed */
            if (name) {
                if ((!$(this).hasClass('nosubmit')
                && $(this).val() !== $(this).data('old')) 
                && (!($(this).data('old') === undefined &&
                $(this).val() === '')))
                {
                    console.log(name + ' has changed from "'
                        + $(this).data('old') + '" to "' + $(this).val()
                        + '"');
                    xml += '<' + name + '>';
                    xml += escapeHTML($(this).val());
                    xml += '</' + name + '>';
                }
            }

            /* close subform tag ? */
            if (subform.length === 1) {
                if ($(this).closest('div.td').is(':last-child')
                && $(this).is(':last-child')) 
                {
                    xml += '</' + tag + '>';
                }
            }
		}
	});
	this.xml = xml;
	this.customXML();
	this.xml += '</' + this.object + '>';
	this.xml += '</data></request>';
	console.log(this.xml);
	/* tell user to wait */
	if (this.action === 'create' || this.action === 'update') {
		showSpinner('Saving ' + this.object + '...');
	}
	else {
		showSpinner();
	}
	this.post();
}

Form.prototype.submitError = function(xhr, s, err) {
	console.log(err);
	hideSpinner();
	if (s === 'timeout') {
		console.log('timeout');
		statusMessage('Timeout. ' + this.object + ' not ' + this.action + 'd.',
			STATUS_CRIT);
	}
	else if (!this.submitErrorCustom(xhr, s, err)) {
		statusMessage('Error saving ' + object, STATUS_CRIT);
	}
}

/* to be overridden by application */
Form.prototype.submitErrorCustom = function(xhr, s, err) {
	return false;
}

Form.prototype.submitSuccess = function(xml) {
	console.log('form submitted successfully');
	hideSpinner();

	TABS.refresh(this.collection); /* refresh any tabs for this collection */

	/* We received some data back. Display it. */
	this.id = $(xml).find('id').text();
	if (!this.id ) console.log('id not found');
    if (this.id) {
        this.data['FORMDATA'] = $(xml);
        if (this.action === 'create') {
            console.log('POST returned new ' + this.object + ' with id='+ this.id);
            this.action = 'update';
            this.sources = this.dataSources();
            this.url += this.id;
            this.html = undefined;
        }
        this.title = tabTitle('Edit ' + this.object + ' ' + this.id,
            this.object, this.action, [xml]);
        this.tab.setTitle(this.title);
        var form = this;
        this.load()
        .done(function() {
            form.show();
        });
    }
}

Form.prototype.updateDataSources = function(data) {
    if (this.sources === undefined) return;
	console.log('Form().updateDataSources()');
	var sources = this.sources;
	console.log('data[' + data.length + ']; sources[' + sources.length + ']');
	for (var i=0; i < sources.length && data[i] !== undefined; i++) {
		console.log('Form().updateDataSources() - saving ' + sources[i]);
		this.data[sources[i]] = data[i][0];
	}
	console.log('Form().updateDataSources() done');
}

/* refresh all totals on form */
Form.prototype.updateAllTotals = function() {
    console.log('Form().updateAllTotals()');
    var form = this;
    var t = this.tab.tablet;
    t.find('input.onchange.multiplicand').each(function() {
        form.rowMultiply($(this));
    });
    t.find('input.onchange.addend').each(function() {
        form.rowSum(this);
    });
}

Form.prototype.updateFormTotals = function(ctl) {
    console.log('Form().updateFormTotals()');
    var t = this.tab.tablet;
    var subtotal = '0.00';
    t.find('.sum').each(function() {
        subtotal = decimalAdd(subtotal, $(this).val());
    });
    var c = t.find('.subtotal');
    if (c.hasClass('currency')) {
        subtotal = String(subtotal).formatCurrency();
    }
    c.val(subtotal);
}

Form.prototype.updateMap = function() {
    console.log('Form().updateMap()');
    if (this.hasMap()) {
        this.map.tab = this.tab;
        this.map.load();
    }
}

Form.prototype.validate = function() {
	console.log('Form().validate()');
    statusHide();

    /* check required fields are filled in */
    var t = this.tab.tablet;
    var b = true;
    t.find('[required="required"]').each(function() {
        if (($(this).is('select') && $(this).val() < 0)
        || ($(this).is('input') && $(this).val() === ''))
        {
            console.log($(this).attr('name'));
            if (b) {
                if ($(this).data('required-message') !== undefined) {
                    statusMessage($(this).data('required-message'),
                        STATUS_WARN);
                }
                else {
                    statusMessage($(this).attr('name') + ' is required',
                        STATUS_WARN);
                }
                $(this).focus();
            }
            b = false;
        }
    });
    /* TODO: check fields with class datefield are dates */
    /* TODO: minOccurs subform items */
    //if (b) b = customFormValidation(this.object, this.action, this.id);
    if (b) b = this.validateCustom();

    return b;
}

/* override in application */
Form.prototype.validateCustom = function() {
    return true;
}

/* Map() */
function Map() {
	if (!(this instanceof Map))
		return new Map();
	this.geodata = new Array();
	this.fields = new Array();
}

Map.prototype.addGeo = function(s) {
    console.log('Map().addGeo(' + s + ')');
	this.geodata.push(s);
}

Map.prototype.clearGeo = function() {
	this.geodata.length = 0;
}

Map.prototype.load = function() {
    console.log('Map().load()');
    if (this.tab === undefined) return; /* not attached to tab */
    if (this.geodata.length > 0) {
        this.geostring = this.geodata.join();
        loadMap(this.geostring, this.tab.id);
    }
}

/* Tab() */
function Tab(title, content, activate, collection, refresh) {
	if (!(this instanceof Tab))
		return new Tab(title, content, activate, collection, refresh);
	console.log('Tab() constructor');

	title = title.substring(0, g_max_tabtitle); /* truncate title */

    console.log('title: ' + title);

	/* if exists, update content */
	var tab = TABS.byTitle[title];
	if (tab) {
		console.log('Tab with title "' + title + '" exists.  Updating.');
		if (content) { tab.setContent(content); }
		if (activate) { tab.activate(); }
		return tab;
	}

	this.title = title;
	TABS.add(this);
	this.business = g_business;
	this.collection = collection;
	this.frozen = false;	/* whether this tab can be refreshed */
	this.refresh = refresh; /* whether this tab should autorefresh */
	this.active = false;
	this.object = null;
	this.action = null;
	console.log('creating new Tab(id=' + this.id + ')');

	/* build array of css classes */
	this.classes = new Array();
	this.classes.push('business' + this.business);
	this.classes.push('tablet' + this.id);
	if (this.collection) {
		this.classes.push(collection);
	}
	if (this.refresh) {
		this.classes.push('refresh');
	}
	var classes = this.classes.join(' ');

	/* create tab in DOM */
	this.tabli =  $('<li/>', { id: "tabli" + this.id, "class": "tabhead" });
	this.tabli.addClass(classes);
    this.tabhead = $('<div/>', { id: "tabhead" + this.id, "class": "tabhead"});
	this.tabtitle = $('<div/>', { "class": "tabtitle" });
	this.tabtitlelink = $('<a/>', { "class": "tabtitle", href: this.id });
	this.tabtitlelink.append(this.title);
	this.tabtitle.append(this.tabtitlelink);
	this.tabx = $('<div/>', { "class": "tabx" });
	this.tabcloser = $('<a/>', {
		id: "tabcloser" + this.id,
		"class": "tabcloser",
		href: this.id
	}).append('X');
	this.tabx.append(this.tabcloser);
	this.tabhead.append(this.tabtitle);
	this.tabli.append(this.tabhead);
	this.tabhead.append(this.tabx);
    $('ul.tablist').append(this.tabli);

	/* set up tab navigation */
	var id = this.id;
	this.tabli.click(function(event) {
		event.preventDefault();
		activateTab(id);
	});

	/* tab closer */
	var tab = this;
	this.tabx.click(function(event) {
		event.preventDefault();
		tab.close();
	});

	/* set content */
	this.tablet = $('<div/>', { id: "tab" + this.id });
	this.tablet.addClass(classes);
	this.tablet.addClass('tablet');
	$('div.tabcontent').append(this.tablet);
	this.setContent(content);

	/* activate */
	if (activate) { this.activate(); }
}

Tab.prototype.activate = function() {
	console.log('Tab().activate()');
	if (this.active) { this.reload(); return false; }
	console.log('activating Tab ' + this.id);

	/* remove "active" styling from all tabs for this business */
	$(".business" + g_business).removeClass('active');

	/* mark selected tab as active */
	$(".tablet" + this.id).addClass("active");

	/* set focus to control with class "focus" */
	var tab = this.tablet;
	tab.find(".focus").focus();

	/* fade in if we aren't already visible */
	$(".tablet" + this.id).find(".focus").fadeIn(300);

	/* update metadata */
	for (var i=0; i < TABS.byId.length; i++) {
		if (TABS.byId[i] != undefined) {
			if (TABS.byId[i].business == g_business) {
				TABS.byId[i].active = false;
			}
		}
	}
	this.active = true;
	TABS.active = this;
	return this;
};

Tab.prototype.close = function() {
	console.log('Tab(' + this.id + ').close()');
	if (this.active) {
		console.log('closing active tab');
		TABS.activateNext();
	}
	$('.tablet' + this.id).remove(); /* remove tab head and content */
	if (TABS.count() == 1) $('div#tabs').fadeOut(300);
	TABS.close(this);
	return this;
};

/* Set up Tab events */
Tab.prototype.events = function() {
	console.log('Tab().events()');
	var tab = this;
	var t = this.tablet;
}

/* perform jquery select on tab contents */
Tab.prototype.find = function(selector) {
	return this.tablet.find(selector);
};

Tab.prototype.reload = function() {
	if (this.collection && !this.frozen && this.business == g_business) {
		console.log('refreshing tab ' + this.id);
		showQuery(this.collection, this.title, this.sort, this.tablet);
	}
	return this;
};

Tab.prototype.setContent = function(content) {
	console.log('tab(' + this.id + ').setContent()');
	if (content == undefined) return;
	var t = this.tablet;
	var statusmsg = t.find('.statusmsg').detach();/* preserve status msg */
	t.empty().append(content);
	if (statusmsg) t.find('.statusmsg').replaceWith(statusmsg);
    this.events();
	return this;
};

Tab.prototype.setTitle = function(title) {
	this.title = title;
	this.tabtitlelink.empty().append(title);
	return this;
};

/* collection of Tab()s */
function Tabs() {
	console.log('Tabs() constructor');
	this.byId = new Array();
	this.byTitle = {};
}

Tabs.prototype.activate = function(id) {
	this.active = id;
	this.byId[id].activate();
}

/******************************************************************************
 * activateNextTab(tabid)
 *
 * Activate the "next" tab.
 *
 * Which tab is next?  Users have come to expect that if they close 
 * the active tab, the next tab to the right will become active,
 * unless there isn't one, in which case we go left instead.
 * See Mozilla Firefox tabs for an example.
 *
 *****************************************************************************/
Tabs.prototype.activateNext = function() {
	console.log("Looking for a tab to activate...");
	/* Try right first */
	for (var i = this.active.id + 1; i < this.byId.length; ++i) {
		console.log("Trying tab " + i);
		if (this.byId[i] != undefined) {
			if (this.byId[i].business == g_business) {
				this.activate(i);
				return true;
			}
		}
	}
	/* now go left */
	for (var i = this.active.id - 1; i >= 0; --i) {
		console.log("Trying tab " + i);
		if (this.byId[i] != undefined) {
			if (this.byId[i].business == g_business) {
				this.activate(i);
				return true;
			}
		}
	}
	return false; /* no tab to activate */
}

Tabs.prototype.add = function(tab) {
    console.log('Tabs().add()');
	tab.id = this.byId.length;
	this.byId.push(tab);
	this.byTitle[tab.title] = tab;
}

Tabs.prototype.close = function(tab) {
	delete this.byId[tab.id];
	delete this.byTitle[tab.title];
	if (tab.active) this.activateNext();
}

Tabs.prototype.count = function() {
	return this.byId.length;
}

/* refresh any tabs in the specified collection (or all if undefined) */
Tabs.prototype.refresh = function(collection) {
	console.log('Tabs().refresh(' + collection + ')');
	for (var i=0; i < this.byId.length; i++) {
		if (TABS.byId[i] != undefined) {
			if (TABS.byId[i].refresh && TABS.byId[i].collection != undefined) {
				if (collection == undefined
				|| collection == TABS.byId[i].collection)
				{
					TABS.byId[i].reload();
				}
			}
		}
	}
}

/* strip any fields listed in array from xml and result the result */
function stripXmlFields(xml, fields) {
	console.log('stripXmlFields()');
	for (var i=0; i<fields.length; i++) {
		xml.find(fields[i]).remove();
	}
	return xml;
}
