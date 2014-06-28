/**
 * fastLiveFilter jQuery plugin 1.0.3
 * 
 * Copyright (c) 2011, Anthony Bush
 * License: <http://www.opensource.org/licenses/bsd-license.php>
 * Project Website: http://anthonybush.com/projects/jquery_fast_live_filter/
 **/

 /**
 * modified by Sebastian Jeckel, 06/2014
 **/

jQuery.fn.fastLiveFilter = function(listStr, options)
{
	options = options || {};
	var groups = options.groups || [];

	var input = this;
	var list = jQuery(listStr);

	var lastFilterStr = "";
	
	var inputTimeout;
	var oldDisplay;

	// Init group stack
	var groupStack = [];
	var stackIndex = 0;

	for (var i=0; i<groups.length; i++)
	{
		var groupData = {}
		groupData.curItem = null;
		groupData.filterFlags = 0; // If a filter term matches the group, that includes all children
		groupData.matchedChild = false; // Groups are shown if any of their children have been matched

		groupStack[i] = groupData;
	}

	var items = list.children()
	var oldDisplay = items.length > 0 ? items[0].style.display : "block";

	var groupPop = function()
	{
		stackIndex--;

		var gd = groupStack[stackIndex];

		// Apply visibility modifiers for current group before changing it
		if (gd.matchedChild)
		{
			setItemVisible(gd.curItem, true);
			groupSetMatchedChild();
		}
	};

	var groupPush = function(item)
	{
		var gd = groupStack[stackIndex];
		gd.curItem = item;
		gd.matchedChild = false;

		// Inherit initial filter flags from parent group
		var t = stackIndex - 1;
		gd.filterFlags = t > -1 ? groupStack[t].filterFlags : 0

		stackIndex++;
	};

	var groupSetFilterFlag = function(index)
	{
		var t = stackIndex - 1;
		if (t > -1)
			groupStack[t].filterFlags |= 1 << index;
	};

	var groupCheckFilterFlag = function(index)
	{
		var t = stackIndex - 1;
		return t > -1 ? ((groupStack[t].filterFlags & (1 << index)) != 0) : false;
	};

	var groupSetMatchedChild = function()
	{
		var t = stackIndex - 1;
		if (t > -1)
			groupStack[t].matchedChild = true;
	};

	var setItemVisible = function(item, isVisible)
	{
		if (! item)
			return;

		if (isVisible)
		{
			if (item.style.display == "none")
				item.style.display = oldDisplay;
		}
		else
		{
			if (item.style.display != "none")
				item.style.display = "none";
		}
	};
	
	var updateInput = function()
	{
		var items = list.children();
		var filters = input.val().toLowerCase().split(" ");

		for (var i=0; i < items.length; i++)
		{
			var item = items[i];
			var itemClass = item.getAttribute("class") || null;
			var innerText = item.textContent || item.innerText || "";

			var groupIndex = itemClass ? groups.indexOf(itemClass) : -1;

			if (groupIndex != -1)
			{
				while (stackIndex > groupIndex)
					groupPop();

				if (stackIndex <= groupIndex)
				{
					while (stackIndex <= groupIndex - 1)
						groupPush(null);

					groupPush(item);
				}
			}

			var shouldShow = true;

			for (var j=0; j < filters.length; j++)
			{
				var filter = filters[j];

				// Pass empty filter strings
				if (filter.length == 0)
					continue;

				// Pass if jth filter matches a group this item is in
				if (groupCheckFilterFlag(j))
					continue;

				//  Pass if filter matches text
				var t = innerText.toLowerCase();
				if (t.indexOf(filter) != -1)
				{
					if (groupIndex != -1)
						groupSetFilterFlag(j);

					continue;
				}

				// Item could not be matched by this filter, don't show it
				shouldShow = false;
				break;
			}

			setItemVisible(item, shouldShow);

			if (shouldShow)
				groupSetMatchedChild();
		}

		// Unwind stack
		while (stackIndex > 0)
			groupPop();
	};

	input.keydown(
		function()
		{
			clearTimeout(inputTimeout);
			inputTimeout = setTimeout(
				function()
				{
					if (input.val() === lastFilterStr)
						return;

					lastFilterStr = input.val();
					updateInput();
				},
				0);
		});

	return this;
}
