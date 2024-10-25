/* 1. Program definition */
program CombinedTest;

/* 2. Variable declaration */
var a : array [1..10] of integer;
var b, c : boolean;
var str : string;

/* 3. Begin block */
begin
    /* 4. Simple operations */
    a[1] := 5;
    if a[1] < 10 then
        b := true;
    else
        b := false;
    
    /* 5. I/O operations */
    write(a[1]);
    readln(c);

    /* 6. Function call */
    procedure myProcedure;
    begin
        writeln('In Procedure');
        return;
    end;

    /* 7. String handling */
    str := 'This is a test string';
    writeln(str);

    /* 8. Loop and break */
    while b do
    begin
        a[1] := a[1] + 1;
        if a[1] > 10 then break;
    end;

    /* End program */
end.
