#include <iostream>
#include <thread>
#include <vector>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fstream>
#include <iomanip> 
#include <string>
#include <sstream>
#include <Python.h>
#include <filesystem>
#include "makecir.hpp"
#include "judge.hpp"
#include "file.hpp"
#include "margin.hpp"
#include "optimize.hpp"


using namespace std;

int main(int argc, const char *argv[]) {
    int shmid;                        // 共有メモリのIDを格納
    vector<judge> jud;                //judgementファイルの各行(開始時間　終了時間　位相) を格納
    vector<int> elej;                 //各判定素子の判定回数を格納
    vector<string> data_cir;          //サーキットファイルを格納
    time_t start, end;                //開始時間と終了時間
    struct tm *tps, *tpe;             //開始時間と終了時間
    char date[100];                   //日付・日時を格納する文字列配列
    board *top, *top2;                //結果を格納する構造体(共有メモリにアタッチする)
    ele_cou *cou;                     //様々な数を格納する構造体(file.hpp 参照)
    vector<ele_unit> element;         //測定素子の情報を格納する構造体を格納する配列
    string filename = argv[1];     //サーキットファイル名　
    string judgefilename = argv[1];   //judgementファイル名
    opt_num *opt;
    vector<string> arg_arr;           //コマンドライン引数が格納されている動的配列
    for (int i = 0; i < argc; i++){   //コマンドライン引数が格納されている静的配列の要素を動的配列に格納
        arg_arr.emplace_back(argv[i]);
    }
    int jud_flg = 0;
    for(int i = 0; i < arg_arr.size(); i++){
        if(jud_flg == 1){
           judgefilename = arg_arr[i];
           cout << judgefilename << endl;
           jud_flg = 0;
        }
        if(arg_arr[i] == "-j"){
            jud_flg = 1;
        }
    }

    start = time(NULL);
    cout << "\n";
    cout << "    MM             MM                                         ii                 'XX         XX'        \n";
    cout << "    MMM           MMM                                                              XXX     XXX         \n";
    cout << "    MMMM         MMMM      ......     ..  ....     .....      ..    .. .....        XXX   XXX         \n";
    cout << "    MM MM       MM MM    .aAAAAAa.    RRrr'''   .gGGGGGgg.    II    NN.nNNNNn.        XXxXX          \n";
    cout << "    MM  MM     MM  MM           AA    RR'      .gg      Gg    II    NN      NN         xXx            \n";
    cout << "    MM   MM   MM   MM     aAAAAAAA    RR       gg.      Gg    II    NN      NN        XXxXX             \n";
    cout << "    MM    MM MM    MM    Aa     aAa   RR       gg.      Gg    II    NN      NN      XXX   XXX           \n";
    cout << "    MM     MMM     MM    Aa     aAa   RR        gg     .Gg    II    NN      NN     XXX     XXX         \n";
    cout << "    MM      m      MM     aAAAAAAa    RR         gGGGGGggg    II    NN      NN   .xX         Xx.         \n";
    cout << "                                                        Gg                                                \n";
    cout << "                                                gg.   .gg                                                  \n";
    cout << "                                                 'gGGGg'                                                   \n";
    cout << "                                                                            Version 2.0               \n";

    cout << " ~~ Parallel Algorithm with Josim ~~ " << endl << endl;
    /*開始時間を表示*/
    time(&start);
    tps = localtime(&start);
    strftime(date, sizeof(date), "%Y/%m/%d %H:%M:%S", tps);
    /*judgementファイルの読み取り*/
    cout << " START : " << date << endl;
    cout << " Checking Judgement File..." << endl;
    if (readJudgementFile(elej, jud, judgefilename) == 0) {
        cout << "can't read JudgementFile" << endl;
        exit(1);
    }
    cout << " Target File Name   : " << judgefilename  << endl;
    cout << " ------PASS------" << endl << endl;
    /*サーキットファイルの読み取り*/
    cout << " Checking CircuitFile..." << endl;
    /*ele_cou構造体のメモリを確保*/
    try{
        cou = new ele_cou;
    }
    catch(bad_alloc) {
        cerr << "can't allocate memory. '*cou' is NULL. " << endl;
        return 0;
    }
    make_data_cir(data_cir, filename, element, cou);
    cout << " Sum of Target      : " << element.size()  << endl;
    cout << " Target File Name   : " << filename  << endl;


    cout << " ------PASS------" << endl << endl;

    int menu_num;
    cout << " Please select an operation mode." << endl << endl;
    cout << " 0. Remove Intermediary File" << endl;
    cout << " 1. Judge" << endl;
    cout << " 2. Calculate Margin" << endl;
    cout << " 3. Optimization with Critical Margin Method" << endl;
    cout << " 4. Optimization with Monte Carlo Method" << endl;
    cout << " 5. Optimization with Monte Carlo and Critical Margin Method" << endl << endl;
    cout << "  Selected mode : ";
    cin >> menu_num;
    cout << endl;

    switch(menu_num){
        case 0:
            arg_arr.emplace_back("-rm");
            break;
        case 1:
            arg_arr.emplace_back("-jc");
            break;
        case 2:
            arg_arr.emplace_back("-m");
            break;
        case 3:
            arg_arr.emplace_back("-c");
            break;
        case 4:
            arg_arr.emplace_back("-om");
            break;
        case 5:
            arg_arr.emplace_back("-op");
            break;
        default:
            cout << " Please input a correct number." << endl;
            return 0;
    }

    time(&start);

    //全対象素子数を格納 
    int sum = element.size(); 
    int menu_flg = 0;
    for (string cmd : arg_arr) {
        if (cmd == "-m") {    // -d があった場合、マージンの詳細情報を表示
            Margin(element, elej, jud, data_cir, cou, arg_arr, 0);
            menu_flg++;
            break;
        }
        else if (cmd == "-c"){     // -f があった場合、 matplotlib を用いたグラフを出力
            critical_margin_method(element, elej, jud, data_cir, cou, arg_arr);
            menu_flg++;
            break;
        }
        else if (cmd == "-op"){     // -f があった場合、 matplotlib を用いたグラフを出力
            optimize(element,data_cir,cou,elej,jud, arg_arr);
            menu_flg++;
            break;
        }
        else if (cmd == "-jc"){     // -f があった場合、 matplotlib を用いたグラフを出力
            stringstream commandname;
            commandname << "josim OPTIMIZE" << getpid() << ".cir > /dev/null";
            make_cir_opt(element, data_cir,cou);
            if(system((commandname.str()).c_str()) == -1){
                cout << "error:1" << endl;
            }
            //正常動作したら
            else if(judge_operation(elej, jud) == 1){ 
                cout << " OK! This Circuit Works Correctly.";
            }
            else{
                cout << " This Circuit Does Not Work Correctly.";
            }
            menu_flg++;
            break;
        }
        else if (cmd == "-om"){     // -f があった場合、 matplotlib を用いたグラフを出力
            optimize_monte(element,data_cir,cou,elej,jud, arg_arr);
            menu_flg++;
            break;
        }
        else if (cmd == "-rm"){     // -f があった場合、 matplotlib を用いたグラフを出力
            system("rm -rf MARGIN*");
            system("rm -rf OPTIMIZE*");
            system("rm -rf CIRCUIT*");
            menu_flg++;
            break;
        }
    
    }

    if(menu_flg == 0){
            cout << " Please select the mode. " << endl;
            return 0;
    }

    cout << " \n\n Finish!!\n\n";
    //終了時間を取得
    time(&end);
    tpe = localtime(&end);
    strftime(date, sizeof(date), "%Y/%m/%d %H:%M:%S", tpe);

      
    /*終了時刻を表示*/
    cout << "\n END      : " << date << endl;

    /*経過時刻を⚪︎時間⚪︎分⚪︎秒のかたちで表示*/
    int hour = 0, min = 0, sec = end - start;
    if (sec >= 3600) {
        hour = sec / 3600;
        sec -= hour * 3600;
    }
    if (sec >= 60) {
        min = sec / 60;
        sec -= min * 60;
    }
    cout.fill('0'); 
    cout << " RUN TIME : " << setw(2) << hour << ":"  << setw(2) << min << ":"  << setw(2) << sec << endl << endl;
    cout.fill(' ');

    //char *delete_MARGIN = "rm -rf MRAGIN*";
    //char *delete_OPTIMIZE = "rm -rf OPTIMIZE*";
    //char *delete_CIRCUIT = "rm -rf CIRCUIT*";

    system("rm -rf MRAGIN*");
    system("rm -rf OPTIMIZE*");
    system("rm -rf CIRCUIT*");

    //動的確保していたメモリを解放
    delete cou;
    return 0;
}
