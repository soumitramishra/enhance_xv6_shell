# enhance_xv6_shell
Add "alias" functionality and logical operators(&amp;&amp; and ||) to xv6 shell

<b> Feature 1: Shell scripts. </b>

If “sh” is run with a single command line argument, it should open that file and read commands from there instead of from the console.<br>
When reading from a script, the shell should not print “$” prompts. <br>
If the script file can’t be opened, the shell should exit with a non-zero status.<br>

<b>Feature 2: && and || operators.</b><br>

These operators operate much like the “;” sequence operator, except the execution of the subcommand on the left is dependent on the exit status of the subcommand on the right.<br>
A command “foo && bar” will execute bar only if foo succeeds (exits with status 0).<br>
A command “foo || bar” will execute bar only if foo fails (exits with a non-zero status).<br>
These operators should be added in a manner consistent with the design of existing operators in the xv6 shell<br>

<b>Feature 2: Alias </b><br>
I added "alias" feature. This is a shortcut for existing commands.
It would help the users to create shorthands for long commands which are to be used frequently.
The user can add new alias, run command using the aliases as well as update existing alias similar to linux bash shell alias command.
For example - "alias t1=status true; done true"
Now this command can be used by typing t1 and pressing enter.
Right now, I have used fixed size([20,20]) two dimenstional arrays to store mappings from alias-name to command-name.
Therefore it supports maximum of 20 aliases each with maximum command-name of 100 characters. 
This can be easily modified in the code by updating the MACRO MAXALIAS in sh.c file.
For the convenience of the the user, I have added "alias -h" and "alias --help" options to display the usage of this command.
