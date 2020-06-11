CLANG_PATH=../cmake-build-debug/bin
CLANG_PATH=../cmake-build-minsizerel/bin
CLANG_PATH=../build/bin



clean:
	@rm -f *.o
	@rm -f *.a
	@rm -f *-32
	@rm -f *-arm
	@rm -f *-aarch64
	@rm -f *-mips
	@rm -f *-mips64el
	@rm -f *-ppc64le
	@rm -f *-amd64
	@rm -f test0
	@rm -f test1
	@rm -f test2
	@rm -f test3
	@rm -f test4
	@rm -f test5
	@rm -f test6
	@rm -f test7
	@rm -f test8
	@rm -f test9
	@rm -f test10
	@rm -f test11
	@rm -f test12
	@rm -f test13
	@rm -f test14
	@rm -f test15
	@rm -f test16
	@rm -f test17
	@rm -f test18
	@rm -f test19
	@rm -f test20
	@rm -f test21
	@rm -f test22
	@rm -f test23
	@rm -f test24
	@rm -f test25
	@rm -f test26
	@rm -f test27
	@rm -f test28
	@rm -f test29
	@rm -f test30
	@rm -f test31
	@rm -f test32
	@rm -f test33
	@rm -f test34
	@rm -f test35
	@rm -f test36
	@rm -f test37
	@rm -f test37_2
	@rm -f test38
	@rm -f test39
	@rm -f test40

clean-ref:
	rm -f *-ref
	rm -f *-ref-32


test1:
	${CLANG_PATH}/clang++ -flto -fno-rtti -O3 -c test1.cpp -o test1.o
	${CLANG_PATH}/llvm-dis test1.o
	${CLANG_PATH}/clang++ -fno-rtti -O3 -flto test1.o -o test1 -fuse-ld=lld


test1-ref:
	clang++-9 test1.cpp -o test1-ref


test1-run: test1 test1-ref
	./test1 a
	./test1-ref a
	./test1 b
	./test1-ref b
	./test1 c
	./test1-ref c


test2:
	${CLANG_PATH}/clang++ -flto -fno-rtti -O2 -c test2.cpp -o test2.o
	${CLANG_PATH}/clang++ -flto -fno-rtti -O2 -c test2a.cpp -o test2a.o
	${CLANG_PATH}/clang++ -flto -fno-rtti -O2 -c test2b.cpp -o test2b.o
	${CLANG_PATH}/clang++ -flto -fno-rtti -O2 -c test2c.cpp -o test2c.o
	${CLANG_PATH}/clang++ -flto -fno-rtti -O2 test2.o test2a.o test2b.o test2c.o -o test2 -fuse-ld=lld


test2-ref:
	clang++-9 -fno-rtti test2.cpp test2a.cpp test2b.cpp test2c.cpp -o test2-ref


test2-run: test2 test2-ref
	./test2 a
	./test2-ref a
	./test2 b
	./test2-ref b
	./test2 c
	./test2-ref c



test3:
	${CLANG_PATH}/clang++ -flto -O3 -c test3.cpp -o test3.o
	${CLANG_PATH}/clang++ -flto -O3 -c sample_classes_decl.cpp -o sample_classes_decl.o
	${CLANG_PATH}/llvm-dis test3.o
	${CLANG_PATH}/clang++ -O3 -flto test3.o sample_classes_decl.o -o test3 -fuse-ld=lld


test3-ref:
	clang++-9 test3.cpp sample_classes_decl.cpp -o test3-ref


test3-run: test3 test3-ref
	./test3 a
	./test3-ref a
	./test3 b
	./test3-ref b
	./test3 c
	./test3-ref c



test4:
	${CLANG_PATH}/clang++ -flto -O3 -c test4.cpp -o test4.o
	${CLANG_PATH}/clang++ -flto -O3 -c test4-impl.cpp -o test4-impl.o
	${CLANG_PATH}/clang++ -O3 -flto test4.o test4-impl.o -o test4 -fuse-ld=lld


test4-ref:
	clang++-9 test4.cpp test4-impl.cpp -o test4-ref


test4-run: test4 test4-ref
	./test4
	./test4-ref


test5:
	${CLANG_PATH}/clang++ -g -flto -O3 -c test5.cpp -o test5.o
	${CLANG_PATH}/clang++ -g -flto -O3 -c test5-impl.cpp -o test5-impl.o
	${CLANG_PATH}/clang++ -g -O3 -flto test5.o test5-impl.o -o test5 -fuse-ld=lld


test5-ref:
	clang++-9 -g test5.cpp test5-impl.cpp -o test5-ref


test5-run: test5 test5-ref
	./test5
	./test5-ref



test5-32:
	${CLANG_PATH}/clang++ -m32 -g -flto -O3 -c test5.cpp -o test5-32.o
	${CLANG_PATH}/clang++ -m32 -g -flto -O3 -c test5-impl.cpp -o test5-impl-32.o
	${CLANG_PATH}/clang++ -m32 -g -O3 -flto test5-32.o test5-impl-32.o -o test5-32 -fuse-ld=lld -no-novt-libs


test5-ref-32:
	clang++-9 -m32 -g test5.cpp test5-impl.cpp -o test5-ref-32


test5-run-32: test5-32 test5-ref-32
	./test5-32
	./test5-ref-32



test6:
	${CLANG_PATH}/clang++ -g -flto -O3 -c test6.cpp -o test6.o
	${CLANG_PATH}/clang++ -g -flto -O3 -c test6-impl.cpp -o test6-impl.o
	${CLANG_PATH}/clang++ -g -O3 -flto test6.o test6-impl.o -o test6 -fuse-ld=lld


test6-ref:
	clang++-9 -g test6.cpp test6-impl.cpp -o test6-ref


test6-run: test6 test6-ref
	./test6
	./test6-ref



test7:
	${CLANG_PATH}/clang++ -flto -O3 -c test7.cpp -o test7.o
	${CLANG_PATH}/clang++ -O3 -flto test7.o -o test7 -fuse-ld=lld

test7-ref:
	clang++-9 test7.cpp -o test7-ref

test7-run: test7 test7-ref
	./test7
	./test7-ref



test8:
	${CLANG_PATH}/clang++ -flto -O3 -c test8.cpp -o test8.o
	${CLANG_PATH}/clang++ -flto -O3 -c test8-impl.cpp -o test8-impl.o
	${CLANG_PATH}/clang++ -O3 -flto test8.o test8-impl.o -o test8 -fuse-ld=lld

test8-ref:
	clang++-9 test8.cpp test8-impl.cpp -o test8-ref

test8-run: test8 test8-ref
	./test8
	./test8-ref



test9:
	${CLANG_PATH}/clang++ -g -flto -O3 -c test9.cpp -o test9.o
	${CLANG_PATH}/clang++ -g -flto -O3 -c test9-impl.cpp -o test9-impl.o
	${CLANG_PATH}/clang++ -g -O3 -flto test9.o test9-impl.o -o test9 -fuse-ld=lld


test9-ref:
	clang++-9 -g test9.cpp test9-impl.cpp -o test9-ref


test9-run: test9 test9-ref
	./test9
	./test9-ref



test10:
	${CLANG_PATH}/clang++ -flto -O3 -c test10.cpp -o test10.o
	${CLANG_PATH}/llvm-dis test10.o
	${CLANG_PATH}/clang++ -O3 -flto test10.o -o test10 -fuse-ld=lld

test10-ref:
	clang++-9 test10.cpp -o test10-ref

test10-run: test10 test10-ref
	./test10
	./test10-ref



test11:
	${CLANG_PATH}/clang++ -flto -O3 -c test11.cpp -o test11.o
	${CLANG_PATH}/clang++ -O3 -flto test11.o -o test11 -fuse-ld=lld

test11-ref:
	clang++-9 test11.cpp -o test11-ref

test11-run: test11 test11-ref
	./test11
	./test11-ref



test12:
	${CLANG_PATH}/clang++ -flto -O3 -c test12.cpp -o test12.o
	${CLANG_PATH}/llvm-dis test12.o
	${CLANG_PATH}/clang++ -O3 -flto test12.o -o test12 -fuse-ld=lld

test12-ref:
	clang++-9 test12.cpp -o test12-ref

test12-run: test12 test12-ref
	./test12
	./test12-ref



test13:
	${CLANG_PATH}/clang++ -flto -O3 -c test13.cpp -o test13.o
	${CLANG_PATH}/llvm-dis test13.o
	${CLANG_PATH}/clang++ -flto -O3 -c test13-impl.cpp -o test13-impl.o
	${CLANG_PATH}/clang++ -O3 -flto test13.o test13-impl.o -o test13 -fuse-ld=lld


test13-ref:
	clang++-9 test13.cpp test13-impl.cpp -o test13-ref


test13-run: test13 test13-ref
	./test13
	./test13-ref



test14:
	${CLANG_PATH}/clang++ -flto -O3 -c test14.cpp -o test14.o
	${CLANG_PATH}/llvm-dis test14.o
	${CLANG_PATH}/clang++ -O3 -flto test14.o -o test14 -fuse-ld=lld


test14-ref:
	clang++-9 test14.cpp -o test14-ref


test14-run: test14 test14-ref
	./test14
	./test14-ref



test15:
	${CLANG_PATH}/clang++ -flto -O3 -c test15.cpp -o test15.o
	${CLANG_PATH}/llvm-dis test15.o
	${CLANG_PATH}/clang++ -O3 -flto test15.o -o test15 -fuse-ld=lld


test15-ref:
	clang++-9 test15.cpp -o test15-ref


test15-run: test15 test15-ref
	./test15
	./test15-ref



test16:
	${CLANG_PATH}/clang++ -flto -O3 -c test16.cpp -o test16.o
	${CLANG_PATH}/llvm-dis test16.o
	${CLANG_PATH}/clang++ -O3 -flto test16.o -o test16 -fuse-ld=lld


test16-ref:
	clang++-9 test16.cpp -o test16-ref


test16-run: test16 test16-ref
	./test16
	./test16-ref



test17:
	${CLANG_PATH}/clang++ -flto -O3 -c test17.cpp -o test17.o
	${CLANG_PATH}/llvm-dis test17.o
	${CLANG_PATH}/clang++ -O3 -flto test17.o -o test17 -fuse-ld=lld


test17-ref:
	clang++-9 test17.cpp -o test17-ref


test17-run: test17 test17-ref
	./test17
	./test17-ref



test18:
	${CLANG_PATH}/clang++ -flto -O3 -c test18.cpp -o test18.o
	${CLANG_PATH}/llvm-dis test18.o
	${CLANG_PATH}/clang++ -O3 -flto test18.o -o test18 -fuse-ld=lld


test18-ref:
	clang++-9 test18.cpp -o test18-ref


test18-run: test18 test18-ref
	./test18
	./test18-ref



test19:
	${CLANG_PATH}/clang++ -flto -O3 -c test19.cpp -o test19.o
	${CLANG_PATH}/llvm-dis test19.o
	${CLANG_PATH}/clang++ -O3 -flto test19.o -o test19 -fuse-ld=lld


test19-ref:
	clang++-9 test19.cpp -o test19-ref


test19-run: test19 test19-ref
	./test19
	./test19-ref



test20:
	${CLANG_PATH}/clang++ -flto -O3 -c test20.cpp -o test20.o
	${CLANG_PATH}/llvm-dis test20.o
	${CLANG_PATH}/clang++ -O3 -flto test20.o -o test20 -fuse-ld=lld


test20-ref:
	clang++-9 test20.cpp -o test20-ref


test20-run: test20 test20-ref
	./test20
	./test20-ref



test21:
	${CLANG_PATH}/clang++ -flto -O2 -fno-rtti -c test21.cpp -o test21.o
	${CLANG_PATH}/clang++ -flto -O2 -fno-rtti -c test21-impl1.cpp -o test21-impl1.o
	${CLANG_PATH}/clang++ -flto -O2 -fno-rtti -c test21-impl2.cpp -o test21-impl2.o
	${CLANG_PATH}/clang++ -flto -O2 -fno-rtti -c test21-impl3.cpp -o test21-impl3.o
	${CLANG_PATH}/llvm-dis test21.o
	#${CLANG_PATH}/clang++ -O2 -flto test21.o test21-impl1.o test21-impl2.o test21-impl3.o -o test21 -fuse-ld=lld
	ar r libtest21.a test21-impl1.o
	${CLANG_PATH}/clang++ -flto -O2 -fno-rtti test21.o test21-impl3.o test21-impl2.o -o test21 -fuse-ld=lld # -L. -ltest21


test21-ref:
	#clang++-9 test21.cpp test21-impl1.cpp test21-impl2.cpp test21-impl3.cpp -o test21-ref
	clang++-9 test21-impl1.cpp -c -o test21-impl1-ref.o -O2 -flto -fno-rtti
	ar r libtest21-ref.a test21-impl1-ref.o
	clang++-9 test21.cpp test21-impl2.cpp test21-impl3.cpp -O2 -flto -fno-rtti -o test21-ref #-L. -ltest21-ref


test21-run: test21 test21-ref
	./test21
	./test21-ref



test22:
	${CLANG_PATH}/clang++ -flto -O3 -c test22.cpp -o test22.o
	${CLANG_PATH}/llvm-dis test22.o
	${CLANG_PATH}/clang++ -O3 -flto test22.o -o test22 -fuse-ld=lld


test22-ref:
	clang++-9 test22.cpp -o test22-ref


test22-run: test22 test22-ref
	./test22
	./test22-ref



test23:
	${CLANG_PATH}/clang++ -std=c++11 -flto -O3 -c test23.cpp -o test23.o
	${CLANG_PATH}/llvm-dis test23.o
	${CLANG_PATH}/clang++ -std=c++11 -O3 -flto test23.o -o test23 -fuse-ld=lld


test23-ref:
	clang++-9 -std=c++11 test23.cpp -o test23-ref


test23-run: test23 test23-ref
	./test23
	./test23-ref



test24:
	${CLANG_PATH}/clang++ -std=c++11 -flto -O3 -c test24.cpp -o test24.o
	${CLANG_PATH}/llvm-dis test24.o
	${CLANG_PATH}/clang++ -std=c++11 -O3 -flto test24.o -o test24 -fuse-ld=lld


test24-ref:
	clang++-9 -std=c++11 test24.cpp -o test24-ref


test24-run: test24 test24-ref
	./test24
	./test24-ref



test25:
	${CLANG_PATH}/clang++ -std=c++11 -flto -O3 -c test25.cpp -o test25.o
	${CLANG_PATH}/llvm-dis test25.o
	${CLANG_PATH}/clang++ -std=c++11 -O3 -flto test25.o -o test25 -fuse-ld=lld


test25-ref:
	clang++-9 -std=c++11 test25.cpp -o test25-ref


test25-run: test25 test25-ref
	./test25
	./test25-ref



test26:
	${CLANG_PATH}/clang++ -std=c++11 -flto -O3 -c test26.cpp -o test26.o
	${CLANG_PATH}/llvm-dis test26.o
	${CLANG_PATH}/clang++ -std=c++11 -O3 -flto test26.o -o test26 -fuse-ld=lld


test26-ref:
	clang++-9 -std=c++11 test26.cpp -o test26-ref


test26-run: test26 test26-ref
	./test26
	./test26-ref



test27:
	${CLANG_PATH}/clang++ -flto -O3 -c test27.cpp -o test27.o
	${CLANG_PATH}/clang++ -flto -O3 -c test27-impl.cpp -o test27-impl.o
	${CLANG_PATH}/clang++ -O3 -flto test27.o test27-impl.o -o test27 -fuse-ld=lld


test27-ref:
	clang++-9 test27.cpp test27-impl.cpp -o test27-ref


test27-run: test27 test27-ref
	./test27
	./test27-ref



test28:
	${CLANG_PATH}/clang++ -flto -O3 -c test28.cpp -o test28.o
	${CLANG_PATH}/clang++ -flto -O3 -c test28-impl1.cpp -o test28-impl1.o
	${CLANG_PATH}/clang++ -flto -O3 -c test28-impl2.cpp -o test28-impl2.o
	${CLANG_PATH}/llvm-dis test28.o
	${CLANG_PATH}/llvm-dis test28-impl1.o
	${CLANG_PATH}/llvm-dis test28-impl2.o
	${CLANG_PATH}/clang++ -O3 -flto test28.o test28-impl1.o test28-impl2.o -o test28 -fuse-ld=lld


test28-ref:
	clang++-9 test28.cpp test28-impl1.cpp test28-impl2.cpp -o test28-ref


test28-run: test28 test28-ref
	./test28
	./test28-ref



test29:
	${CLANG_PATH}/clang++ -flto -O3 -c test29.cpp -o test29.o
	${CLANG_PATH}/clang++ -flto -O3 -c test29-impl.cpp -o test29-impl.o
	${CLANG_PATH}/llvm-dis test29.o
	${CLANG_PATH}/llvm-dis test29-impl.o
	${CLANG_PATH}/clang++ -O3 -flto test29.o test29-impl.o -o test29 -fuse-ld=lld


test29-ref:
	clang++-9 test29.cpp test29-impl.cpp -o test29-ref


test29-run: test29 test29-ref
	./test29
	./test29-ref



test30:
	${CLANG_PATH}/clang++ -std=c++11 -flto -O3 -c test30.cpp -o test30.o
	${CLANG_PATH}/llvm-dis test30.o
	${CLANG_PATH}/clang++ -std=c++11 -O3 -flto test30.o -o test30 -fuse-ld=lld


test30-ref:
	clang++-9 -std=c++11 test30.cpp -o test30-ref


test30-run: test30 test30-ref
	./test30
	./test30-ref



test31:
	${CLANG_PATH}/clang++ -std=c++11 -flto -O3 -g -c test31.cpp -o test31.o
	${CLANG_PATH}/llvm-dis test31.o
	${CLANG_PATH}/clang++ -std=c++11 -O3 -flto -g test31.o -o test31 -fuse-ld=lld


test31-ref:
	clang++-9 -std=c++11 test31.cpp -o test31-ref


test31-run: test31 test31-ref
	./test31
	./test31-ref



test32:
	${CLANG_PATH}/clang++ -std=c++11 -flto -O3 -g -c test32.cpp -o test32.o
	${CLANG_PATH}/llvm-dis test32.o
	${CLANG_PATH}/clang++ -std=c++11 -O3 -flto -g test32.o -o test32 -fuse-ld=lld -no-novt-libs


test32-ref:
	clang++-9 -std=c++11 test32.cpp -o test32-ref


test32-run: test32 test32-ref
	./test32
	./test32-ref



test33:
	${CLANG_PATH}/clang++ -std=c++11 -flto -O3 -g -c test33.cpp -o test33.o
	${CLANG_PATH}/llvm-dis test33.o
	${CLANG_PATH}/clang++ -std=c++11 -O3 -flto -g test33.o -o test33 -fuse-ld=lld -no-novt-libs


test33-ref:
	clang++-9 -std=c++11 test33.cpp -o test33-ref


test33-run: test33 test33-ref
	./test33
	./test33-ref



test34:
	${CLANG_PATH}/clang++ -std=c++11 -flto -O3 -g -c test34.cpp -o test34.o
	${CLANG_PATH}/llvm-dis test34.o
	${CLANG_PATH}/clang++ -std=c++11 -O3 -flto -g test34.o -o test34 -fuse-ld=lld -no-novt-libs


test34-ref:
	clang++-9 -std=c++11 test34.cpp -o test34-ref


test34-run: test34 test34-ref
	./test34
	./test34-ref



test35:
	${CLANG_PATH}/clang++ -std=c++11 -flto -O3 -g -c test35.cpp -o test35.o
	${CLANG_PATH}/llvm-dis test35.o
	${CLANG_PATH}/clang++ -std=c++11 -O3 -flto -g test35.o -o test35 -fuse-ld=lld -no-novt-libs


test35-ref:
	clang++-9 -std=c++11 test35.cpp -o test35-ref


test35-run: test35 test35-ref
	./test35
	./test35-ref



test36:
	${CLANG_PATH}/clang++ -std=c++11 -flto -O3 -g -c test36.cpp -o test36.o
	${CLANG_PATH}/llvm-dis test36.o
	${CLANG_PATH}/clang++ -std=c++11 -O3 -flto -g test36.o -o test36 -fuse-ld=lld -no-novt-libs


test36-ref:
	clang++-9 -std=c++11 test36.cpp -o test36-ref


test36-run: test36 test36-ref
	./test36
	./test36-ref



test37:
	${CLANG_PATH}/clang++ -std=c++11 -flto -O1 -g -c test37.cpp -o test37.o
	${CLANG_PATH}/llvm-dis test37.o
	${CLANG_PATH}/clang++ -std=c++11 -O1 -flto -g test37.o -o test37 -fuse-ld=lld -no-novt-libs


test37-ref:
	clang++-9 -std=c++11 test37.cpp -o test37-ref


test37-run: test37 test37-ref
	./test37
	./test37-ref



test38:
	${CLANG_PATH}/clang++ -std=c++11 -flto -O1 -g -c test38.cpp -o test38.o
	${CLANG_PATH}/llvm-dis test38.o
	${CLANG_PATH}/clang++ -std=c++11 -O1 -flto -g test38.o -o test38 -fuse-ld=lld -no-novt-libs


test38-ref:
	clang++-9 -std=c++11 test38.cpp -o test38-ref


test38-run: test38 test38-ref
	./test38
	./test38-ref


