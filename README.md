# Bugmatic

## What is it?

Bugmatic is supposed to be a simple offline browser/editor for Github Issues.
The idea is that just as you can download your Git reporsitory from Github
and fully use it while you are on a plane, you should be able to manage your
Github Issues.

## Usage example:

	mac:~ user$ mkdir mybugs
	mac:~ user$ cd mybugs
	mac:~ user$ bugmatic init
	mac:~ user$ bugmatic new "I have issues!" "Let's fix them."
	mac:~ user$ bugmatic push userName projectName
	
or if you already have issues on Github:

	mac:~ user$ mkdir mybugs
	mac:~ user$ cd mybugs
	mac:~ user$ bugmatic clone userName projectName
	mac:~ user$ bugmatic list WHERE assignee.login=userName
	#4: Add support for OAuth
	#5: Should report rate limit exceeded
	mac:~ user$ bugmatic new "Fix this, too?" "I just thought of it."
	mac:~ user$ bugmatic push userName projectName

## How does it work

Currently, there's a macOS Xcode project that builds a `bugmatic` command line
tool. You use this tool to manage your issues, however all state is saved in a
folder of JSON files on your disk, so you should not have any problems accessing
your bugs even if you do not have Bugmatic.

The `bugmatic` tool can currently download a full copy of your Github issues
and their comments. It can also create new local bugs, and create simple bugs
on Github. Eventually, it will be able to upload new local bugs to Github and
sync changes from Github back down.

Ideally, we would use Git as our backing store, or failing that, behave as
much as Git as we can.

## License

	Copyright 2016 by Uli Kusterer.
	
	This software is provided 'as-is', without any express or implied
	warranty. In no event will the authors be held liable for any damages
	arising from the use of this software.
	
	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:
	
	   1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.
	
	   2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.
	
	   3. This notice may not be removed or altered from any source
	   distribution.
