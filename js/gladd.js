/* 
 * gladd.js - gladd helper functions
 *
 * this file is part of GLADBOOKS
 *
 * Copyright (c) 2012, 2013 Brett Sheffield <brett@gladbooks.com>
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
var g_authtimeout = 60000; /* milliseconds - every 60s */
var g_resourcedefaultsurl = '/defaults/';
var g_username = '';
var g_password = '';
var g_instance = '';
var g_business = '1';
var g_loggedin = false;
var g_tabid = 0;
var g_max_tabtitle = '48'; /* max characters to allow in tab title */
var g_session = false;

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
		$(this).fadeTo("slow", 0, function(){location.reload(true);});
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

});

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
		beforeSend: function (xhr) { setAuthHeader(xhr); },
		success: function(data) { loginok(data); },
		error: function(data) { loginfailed(); }
	});
}

function auth_session_logout()
{
	console.log('session logout...');
	g_session = false;
	$.ajax({
		url: g_authurl,
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
		closeTab($(this).attr('href'));
	});
}

/*****************************************************************************/
/* add a new tab with content, optionally activating it */
function addTab(title, content, activate, collection, refresh) {
	console.log('addTab()');
	var tab = newtab();
	tab.title = title;
	tab.content = content;
	tab.activate = activate;
	tab.collection = collection;
	tab.refresh = refresh;

	if (tab.collection) {
		tab.contentClasses += ' ' + tab.collection;
	}
	if (refresh) {
		tab.contentClasses += ' refresh';
	}

	/* truncate tab title if required */
	tab.title = tab.title.substring(0, g_max_tabtitle);

	/* check if this tab already exists */
	var oldtabid = getTabByTitle(tab.title);
	if (oldtabid >= 0) {
		updateTab(oldtabid, tab.content, tab.activate);
		return oldtabid;
	}

	/* add tab and closer */
	$('ul.tablist').append('<li id="tabli' + tab.id
		+ '" class="' + tab.classes + '">'
		+ '<div id="tabhead' + tab.id + '" class="tabhead">'
		+ '<div class="tabtitle">'
		+ '<a class="tabtitle" href="' + tab.id + '">' + tab.title + '</a>'
		+ '</div>'
		+ '<div class="tabx">'
		+ '<a id="tabcloser' + tab.id + '" class="tabcloser" href="'
		+ tab.id  + '">'
		+ 'X</a>'
		+ '</div>' 
		+ '</li>');

	/* add content */
	$('div.tabcontent').append('<div id="tab' + tab.id + '" '
		+ 'class="' + tab.contentClasses + '">');
	$('div#tab' + tab.id).append(tab.content);

	/* store metadata */
	setTabMeta(tab.id, 'id', tab.id);
	setTabMeta(tab.id, 'instance', g_instance);
	setTabMeta(tab.id, 'business', g_business);
	setTabMeta(tab.id, 'collection', collection);

	/* add closer event */
    $('#tabcloser' + tab.id).click(function(event) {
		event.preventDefault();
		closeTab(tab.id);
	});

	/* set up tab navigation */
	$(".tablist li").click(function(event) {
		event.preventDefault();
		var selected_tab = $(this).find("a").attr("href");
		activateTab(selected_tab);
	});
	
	/* activate our new tab */
	if (tab.activate) {
		activateTab(tab.id);
	}

    /* set click events */
	getTabById(tab.id).find('a.tablink').each(function() {
		$(this).click(clickTabLink);
	});

	/* fade in if we aren't already visible */
	$('div.tabs').fadeIn(300);

	return tab.id; /* return new tab id */
}

function setTabMeta(tab, key, value) {
    if (isTabId(tab)) {
		$('div#tabhead' + tab).data(key, value);
	}
	else if (typeof tab == 'object') {
		tab.data(key, value);
	}
    else { /* something else we weren't expecting */
        console.log(typeof tab + ' passed to setTabMeta()');
        return false;
    }
}

function getTabMeta(tab, key) {
    if (isTabId(tab)) {
        return $('div#tabhead' + tab).data(key);
    }
    else if (typeof tab == 'object') {
        return tab.data(key);
    }
    else { /* something else we weren't expecting */
        console.log(typeof tab + ' passed to getTabMeta()');
        return false;
    }
}

function clearTabMeta(tab) {
    if (isTabId(tab)) {
		$('div#tabhead' + tab).removeData();
	}
	else if (typeof tab == 'object') {
		tab.removeData();
	}
    else { /* something else we weren't expecting */
        console.log(typeof tab + ' passed to clearTabMeta()');
        return false;
    }
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
		console.log('updating tab ' + o);
		var tab = $('#tab' + o);
		var tabid = o;
	}
	else if (typeof o == 'object'){ /* jquery object (probably) */
		var tab = o;
		var tabid = getTabMeta(o, 'id');
	}
	else { 							/* unexpected type */
		console.log(typeof o + ' passed to updateTab()');
		return false;
	}

	var statusmsg = tab.find('.statusmsg').detach();/* preserve status msg */
	tab.empty().append(content);					/* replace content */
	clearTabMeta(o);								/* clear metadata */
	tab.find('a.tablink').click(clickTabLink);		/* set click events */

	if (title) setTabTitle(tabid, title);
	if (statusmsg) tab.find('.statusmsg').replaceWith(statusmsg);
	if (activate) activateTab(tabid);

	console.log('finished updating tab ' + tabid);

	/* return tab id, or the object we started with */
	return (tabid) ? tabid : o;
}

function setTabTitle(tabid, title) {
	$('a.tabtitle[href="' + tabid + '"]').text(title);
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
		console.log("activating tab " + tabid);
        /* remove "active" styling from all tabs for this business */
        $(".tabhead.business" + g_business).removeClass('active');
        $(".tablet.business" + g_business).removeClass('active');

        /* mark selected tab as active */
        $(".tablet" + tabid).addClass("active");

		/* set focus to control with class "focus" */
        $(".tablet" + tabid).find(".focus").focus();
	}
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
function activateNextTab(tabid) {
	var trytab = tabid + 1;

	console.log("Looking for a tab to activate...");
	/* Try right first */
	while (trytab < g_tabid) {
		console.log("Trying tab " + trytab);
		if ($('.tablet' + trytab).length != 0) {
			if ($('.tablet' + trytab).hasClass('business' + g_business)) {
				activateTab(trytab);
				return true;
			}
		}
		trytab++;
	}
	/* now go left */
	trytab = tabid - 1;
	while (trytab >= 0) {
		console.log("Trying tab " + trytab);
		if ($('.tablet' + trytab).length != 0) {
			if ($('.tablet' + trytab).hasClass('business' + g_business)) {
				activateTab(trytab);
				return true;
			}
		}
		trytab--;
	}
	return false; /* no tab to activate */
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
function setAuthHeader(xhr) {
	if (!g_session) {
		console.log('setting auth header');
		var hash = auth_encode(g_username, g_password);
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

	g_instance = '';
	$(xml).find('instance').each(function() {
		g_loggedin = true;
		g_instance = $(this).text();
		console.log('Instance selected: ' + g_instance);
	});
	if (g_instance == '') {
		/* couldn't find instance for user - treat as failed login */
		loginfailed();
	}
	else {
		/* have instance, hide login dialog and get list of businesses */
		hideLoginBox();
		prepBusinessSelector();
	}
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

function clickTabLink(event) {
	event.preventDefault();
	showHTML($(this).attr("href"), 'Help' , false);
}

/*****************************************************************************/
/* Display query results as list */
function showQuery(collection, title, sort, tab) {
	showSpinner();
	$.ajax({
		url: collection_url(collection),
		beforeSend: function (xhr) { setAuthHeader(xhr); },
		success: function(xml) {
			displayResultsGeneric(xml, collection, title, sort, tab);
		},
		error: function(xml) {
			displayResultsGeneric(xml, collection, title, sort, tab);
		}
	});
}

/*****************************************************************************/
/* Fetch HTML fragment and display in new tab */
function showHTML(url, title, tab, collection) {
	if (collection) url = collection_url(url);
	showSpinner();
	return $.ajax({
		url: url,
		beforeSend: function (xhr) { setAuthHeader(xhr); },
		success: function(html) {
			if (tab) {
				tab = updateTab(tab, html);
			}
			else {
				tab = addTab(title, html, true);
			}
			hideSpinner();
		},
		error: function() {
			if (tab) {
				tab = updateTab(tab, 'Not found.');
			}
			else {
				tab = addTab(title, 'Not found.', false);
			}
			hideSpinner();
		}
	});
}

/******************************************************************************/
/* fetch html form from server to display */
/* TODO: clean up - xml arg no longer needed? */
function getForm(object, action, title, xml, tab) {
	console.log('getForm()');
	showSpinner(); /* tell user to wait */

	var d = fetchFormData(object, action);
	return $.when.apply(null, d)
	.done(function(html) {
		var args = Array.prototype.splice.call(arguments, 1);
		var safeHTML = $.parseHTML(html[0]);
		cacheData(object, action, args);
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
	var locString = '';
	var id = '';
	var mytab = getTabById(tab);

	if (xml) {
		/* we have some data, pre-populate form */
		$(xml[0]).find('resources').find('row').children().each(function() {
			var tagName = this.tagName;
			var tagValue = $(this).text();

			if ((tagName == 'id') || (tagName == object)) {
				id = tagValue;
			}

			/* set field value */
			var fld = mytab.find('form.' + object).find(
				"[name='" + tagName + "']"
			);
			fld.val(tagValue);
			fld.trigger("liszt:updated");/* ensure chosen type combos update */

			/* get location data, giving preference to postcode */
			if ((tagName == 'town') || (tagName == 'postcode')) {
				if (tagValue.length > 0) {
					locString = tagValue;
				}
			}
		});

		/* load map */
		if (locString.length > 0) {
			console.log('locString:' + locString);
			loadMap(locString, tab);
		}
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

function addOrUpdateTab(tab, content, activate, title) {
    if (typeof tab == 'undefined') {
		tab = addTab(title, content, activate);
	}
	else {
		tab = updateTab(tab, content, activate, title);
	}
	return tab;
}

/*****************************************************************************/
/* display html form we've just fetched in new tab */
function displayForm(object, action, title, html, xml, tab) {
	console.log('displayForm("'+ object +'","'+ action +'","'+ title +'")');
	var x = (action == 'update') ? 2 : 0; /* which xml to start with */

	title = tabTitle(title, object, action, xml);
	tab = addOrUpdateTab(tab, html, true, title);

	/* add some metadata */
	setTabMeta(tab, 'object', object);
	setTabMeta(tab, 'action', action);

	/* tab is either a numeric value or a jquery object */
	mytab = (isTabId(tab)) ? getTabById(tab) : tab;

	/* populate combos with xml data */
	mytab.find('select.populate:not(.sub)').each(function() {
		populateCombo(xml[x++], $(this), tab);
	});

	/* pre-populate form */
	var id = (action == 'update') ? populateForm(tab, object, xml) : 0;

	/* deal with subforms and finalise form display */
	handleSubforms(tab, html, id, xml);
	finaliseForm(tab, object, action, id);
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
		closeTab();
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

function csvToXml(sha) {
	showSpinner('Converting csv to xml...');
	$.ajax({
		url: collection_url('csvtoxml/' + sha),
		beforeSend: function (xhr) { setAuthHeader(xhr); },
		dataType: 'xml',
		success: function(xml) {
			xml = fixXMLDates(xml);
			xml = fixXMLRequest(xml);
			postBankData(xml);
		},
		error: function(xml) {
			displayResultsGeneric(xml, collection, title);
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
$.fn.populate = function(tab) {
	$(this).each(function() { /* wrapped in case many objects selected */
		$(this)._populate(tab);
	});
}
$.fn._populate = function(tab) {
	if (tab == undefined) tab = activeTab();
	var combo = $(this);
	var datasource = $(this).attr('data-source');
	var xml = tab.data('data-' + datasource);
	var selections = [];

	console.log('$.populate()');
	/* FIXME: data frequently can't be found */
	console.log('populating combo from datasource: ' + datasource);
	if (!xml) {
		/* data lost: go fetch it now, and wait for it */
		console.log('datasource not found, fetching it');
		showSpinner();
		getXML(collection_url(datasource), false).done(function(xdata) {
			xml = xdata;
			tab.data('data-' + datasource, xml);
			hideSpinner();
		});
	}

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
}

/* save form data sources to object */
$.fn.updateDataSources = function(data) {
	console.log('$.updateDataSources()');
	var object = $(this).data('object');
	var action = $(this).data('action');
	var sources = dataSources(object, action);
	for (var i in  sources) {
		console.log('updateDataSources() - saving ' + sources[i]);
		$(this).data('data-' + sources[i], data[i].responseText);
	}
}

function dataSources(object, action) {
	return $.grep(g_formdata, function(n, i) {
		return (n[0] == object && n[1] == action);
	})[0][2];
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
	xml += '<' + object 
	if (id > 0) {
		xml += ' id="' + id + '"';
	}
	xml += '>';
	mytab.find(
		'div.' + object
	).find('input:not(.nosubmit,default),select:not(.nosubmit,.default)').each(function() {
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
				console.log('processing input ' + name);
				if ((name != 'id') && (name != 'subid')
				&& ((name != 'relationship')||(object == 'organisation_contacts')))
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
						if ($(this).val()) {
							if ($(this).val().length > 0) { /* skip blanks */
								xml += '<' + name + '>';
								xml += escapeHTML($(this).val());
								xml += '</' + name + '>';
							}
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

	/* send request */
    $.ajax({
        url: url,
        type: 'POST',
        data: xml,
        contentType: 'text/xml',
		beforeSend: function (xhr) { setAuthHeader(xhr); },
        success: function(xml) { submitFormSuccess(object, action, id, collection, xml); },
        error: function(xml) { submitFormError(object, action, id); }
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

	// lets check for tabs that will need refreshing
	$('div.refresh.' + collection).each(function() {
		/* TODO: ensure tab is within current business */
		var tabid = $(this).attr('id').substr(3);
		var title = $('#tabli' + tabid).find(
			'a[href="' + tabid + '"]:not(.tabcloser)'
		).text();
		showQuery(collection, title, false, tabid);
	});

	/* We received some data back. Display it. */
	newid = $(xml).find(object).text();
	if (newid) {
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
		getForm(object, action, null, null, activeTabId());
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
		beforeSend: function (xhr) { setAuthHeader(xhr); }
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
		var eo = oddEven(row);
        tr = $('<div class="tr ' + eo + '"/>');
        formtable.append(tr);
		$(this).children().each(function() {
			var n = 'xml-' + this.tagName;
			var v = ($(this).text() != '') ? $(this).text() : '&nbsp;';
			var td = $('<div class="td ' + eo + ' ' + n + '">' + v + '</div>');
			tr.append(td);
		});
    });
	var clearfix = $('<div class="clearfix"/>');
	formtable.append(clearfix);
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

function selectRowSingular(o) {
	deselectAllRows(o);
	toggleSelected(o);
}

/* build a basic accordion object */
function accordionize(o) {
	o.find('h3').each(function() {
		$(this).click(accordionClick);
	});
}

function accordionClick() {
	if ($(this).next().visible) return;
	$(this).parent().find('div').hide();
	$(this).parent().find('h3').removeClass('selected');
	$(this).addClass('selected');
	$(this).next().fadeIn();
}

$.fn.accordionTab = function(n) {
    return $(this).find('div.accordion h3:nth-child(' + n + ')');
}

$.fn.accordionTabSelect = function(n) {
    $(this).accordionTab(n).each(accordionClick);
}


/*****************************************************************************/
/* display XML results as a sortable table */
function displayResultsGeneric(xml, collection, title, sorted, tab, headers) {
	var refresh = false;

	statusHide();

	/* TODO: refactor */
	if (collection == 'contacts') {
		refresh = true;
	}
	else if (collection == 'organisations') {
		refresh = true;
	}
	else if (collection == 'products') {
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
				addTab(title, $t, true, collection, refresh);
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
	var collection = getTabMeta(activeTabId(), 'collection');
	if (collection == 'salesinvoices') {
		/* view salesinvoice pdf */
		var a=row.find('td.xml-pdf').find('a');
		var href=a.attr('href');
		var si = a.attr('id');
		var html = '<div class="pdf">';
		html += '<object class="pdf" data="' + href + '"';
		html += 'type="application/pdf">';
		html += 'alt : <a href="' + href + '">PDF</a>';
		html += '</object></div>';
		addTab(si, html, true);
	}
	else {
		if (collection == 'accounts') {
			var id = row.find('td.xml-nominalcode').text();
		}
		else {
			var id = row.find('td.xml-id').text();
		}
		if (collection == 'reports/accountsreceivable') {
			var title = 'Statement: ' + row.find('td.xml-orgcode').text();
		}
		else {
			var title = null;
		}
		displayElement(collection, id, title);
	}
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
	url =  '/' + g_instance + '/' + g_business + '/' + collection + '/';
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
			getForm('business', 'create', 'Add New Business');
		}
	});
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
