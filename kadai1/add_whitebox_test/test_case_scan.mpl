// Test valid program structure with comments and multiple tokens
program TestProgram;
var x := 10;  // Test variable assignment and single-line comment

/*
Test block comment handling
and multi-line comment skipping
*/
x := x + 1;

if x >= 10 then
    writeln('x is 10 or more');
    
// Test comparison operators
if x < 20 then
    writeln('x is less than 20');

// Test string literal
var str := 'Hello, world!';

// Test unterminated string literal
// var str2 := 'This string is not closed

// Test square brackets for array indexing
var arr[10] := 0;

// Test handling of a long identifier name exceeding limits
var longIdentifierNameThatIsReallyLongAndExceedsTheMaximumAllowedSize := 10;

/* End of test case */

