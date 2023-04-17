#include <iostream>
#include "logging.h"
#include "logging.c"
#include "message.c"
#include <vector>
#include <string>
#include <poll.h>
#include <sys/select.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

struct bomb_datas {
    unsigned int radius;
    coordinate position;
};

int main(){
    int map_width, map_height, obstacle_count, bomber_count;
    cin >> map_width >> map_height >> obstacle_count >> bomber_count;

    //create the obstacles vector
    vector<obsd> obstacles;
    for(int i = 0; i < obstacle_count; i++){
        obsd obstacle;
        cin >> obstacle.position.x >> obstacle.position.y >> obstacle.remaining_durability;
        obstacles.push_back(obstacle);
    }
    //create the bombers vector
    vector<od> bombers;
    vector <vector<string>> bomber_args;
    for(int i = 0; i < bomber_count; i++){
        od bomber;
        int arg_count;

        cin >> bomber.position.x >> bomber.position.y >> arg_count; 

        bomber.type = BOMBER;
        vector<string> args;
        for(int j = 0; j < arg_count; j++){
            string arg;
            cin >> arg;
            args.push_back(arg);
        }

        bomber_args.push_back(args);
        bombers.push_back(bomber);
    }

    bool game_over = false;

    int bomber_fd[bomber_count][2];
    struct pollfd poll_fds[bomber_count];
    int ret;

    bool bomber_dead[bomber_count];
    for(int i = 0; i < bomber_count; i++){
        bomber_dead[i] = false;
    }
    int alive_bombers = bomber_count;
    bool bomber_win[bomber_count];
    for(int i = 0; i < bomber_count; i++){
        bomber_win[i] = false;
    }

    vector <int*> bomb_fds;
    vector <struct pollfd> bomb_poll_fds;


    for (int i = 0; i < bomber_count; i++)
    {
        if(PIPE(bomber_fd[i]) == -1){
            perror("pipe");
            exit(1);
        }

        poll_fds[i].fd = bomber_fd[i][0];
        poll_fds[i].events = POLLIN;
    }
    
    vector <pid_t> bomber_pids;
    vector <pid_t> bomb_pids;

    for(int i = 0; i < bomber_count; i++){
        bomber_pids.push_back(fork());
        if(bomber_pids.back()){
            close(bomber_fd[i][1]);

        }
        else{
            close(bomber_fd[i][0]);
            bomber_pids[i] = getpid();
            dup2(bomber_fd[i][1], 1);
            dup2(bomber_fd[i][1], 0);
            vector<string> bargs = bomber_args[i];
            vector<char*> cargs(bargs.size() + 1);
            for (size_t j = 0; j < bargs.size(); ++j) {
                cargs[j] = const_cast<char*>(bargs[j].c_str());
            }
            cargs[bargs.size()] = nullptr;

            execv(bargs[0].c_str(), cargs.data());
            std::cerr << "Error: execv failed\n";
            exit(1);
        }
    }
    
    vector <bomb_datas> bombs;

    while (!game_over)
    {
        ret = poll(poll_fds, bomber_count, 1000);
        if (ret == -1)
        {
            perror("poll");
            exit(1);
        }
        
        for(int i = 0; i < bomber_count; i++){ //check if any of the pipes has data, if so read it and act accordingly
            if(poll_fds[i].revents & POLLIN){
                if(alive_bombers == 1){
                    bomber_win[i] = true;
                    game_over = true;
                }
                if(bomber_win[i]){
                    //send BOMBER_WIN message

                    om out;
                    out.type = BOMBER_WIN;
                    ret =  write(bomber_fd[i][0], &out, sizeof(om));
                    if(ret == -1){
                        perror("write");
                        exit(1);
                    }
                    omp omtp;
                    omtp.pid = bomber_pids[i];
                    omtp.m = &out;
                    print_output(NULL, &omtp, NULL, NULL);
                }
                if(bomber_dead[i]){
                    //send BOMBER_DIE message
                    om out;
                    out.type = BOMBER_DIE;
                    ret =  write(bomber_fd[i][0], &out, sizeof(om));
                    if(ret == -1){
                        perror("write");
                        exit(1);
                    }
                    omp omtp;
                    omtp.pid = bomber_pids[i];
                    omtp.m = &out;
                    print_output(NULL, &omtp, NULL, NULL);
                    waitpid(bomber_pids[i], NULL, 0);
                    //TODO: kill the bomber process
                }
                if(bomber_dead[i] || bomber_win[i]){
                    continue;
                }

                im in;
                ret = read(bomber_fd[i][0], &in, sizeof(im));
                if(ret == -1){
                    perror("read");
                    exit(1);
                }
                imp imtp;
                imtp.pid = bomber_pids[i];
                imtp.m = &in;
                print_output(&imtp, NULL, NULL, NULL);
                //cerr << "Bomber " << i << " ,Incoming message type: "<< in.type << "\n" <<endl;

                om out;
                if(in.type == BOMBER_START){
                    out.type = BOMBER_LOCATION;
                    out.data.new_position = bombers[i].position;
                    //cerr << "Bomber" << i << " started at location: " << bombers[i].position.x << " " << bombers[i].position.y << endl;

                    ret =  write(bomber_fd[i][0], &out, sizeof(om));
                    if(ret == -1){
                        perror("write");
                        exit(1);
                    }
                    
                    omp omtp;
                    omtp.pid = bomber_pids[i];
                    omtp.m = &out;
                    print_output(NULL, &omtp, NULL, NULL);

                }
                else if(in.type == BOMBER_SEE){ 
                    out.type = BOMBER_VISION;
                    int obj_count=0;
                    vector <od> objects_around;
                    for(int j = 0; j < obstacle_count; j++){    //check for obstacles
                        for (int k = 1; k <= 3; k++) //right side
                        {
                            if(obstacles[j].position.x == bombers[i].position.x + k && obstacles[j].position.y == bombers[i].position.y){
                                od obs;
                                obs.position = obstacles[j].position;
                                obs.type = OBSTACLE;
                                objects_around.push_back(obs);
                                obj_count++;
                                break;
                            }
                        }
                        for (int k = 1; k <= 3; k++) //left side
                        {
                            if(obstacles[j].position.x == bombers[i].position.x - k && obstacles[j].position.y == bombers[i].position.y){
                                od obs;
                                obs.position = obstacles[j].position;
                                obs.type = OBSTACLE;
                                objects_around.push_back(obs);
                                obj_count++;
                                break;
                            }
                        }
                        for (int k = 1; k <= 3; k++) //up side
                        {
                            if(obstacles[j].position.x == bombers[i].position.x && obstacles[j].position.y == bombers[i].position.y + k){
                                od obs;
                                obs.position = obstacles[j].position;
                                obs.type = OBSTACLE;
                                objects_around.push_back(obs);
                                obj_count++;
                                break;
                            }
                        }
                        for (int k = 1; k <= 3; k++) //down side
                        {
                            if(obstacles[j].position.x == bombers[i].position.x && obstacles[j].position.y == bombers[i].position.y - k){
                                od obs;
                                obs.position = obstacles[j].position;
                                obs.type = OBSTACLE;
                                objects_around.push_back(obs);
                                obj_count++;
                                break;
                            }
                        }
                    }

                    for(int j = 0; j < bomber_count; j++){ //check for other bombers
                        for(int k = 0; k < 4; k++){ //right side
                            if(bombers[j].position.x == bombers[i].position.x + k && bombers[j].position.y == bombers[i].position.y && j != i){
                                od bmr;
                                bmr.position = bombers[j].position;
                                bmr.type = BOMBER;
                                objects_around.push_back(bmr);
                                obj_count++;
                                break;
                            }
                        }
                        for(int k = 0; k < 4; k++){ //left side
                            if(bombers[j].position.x == bombers[i].position.x - k && bombers[j].position.y == bombers[i].position.y && j != i){
                                od bmr;
                                bmr.position = bombers[j].position;
                                bmr.type = BOMBER;
                                objects_around.push_back(bmr);
                                obj_count++;
                                break;
                            }
                        }
                        for(int k = 0; k < 4; k++){ //up side
                            if(bombers[j].position.x == bombers[i].position.x && bombers[j].position.y == bombers[i].position.y + k && j != i){
                                od bmr;
                                bmr.position = bombers[j].position;
                                bmr.type = BOMBER;
                                objects_around.push_back(bmr);
                                obj_count++;
                                break;
                            }
                        }
                        for(int k = 0; k < 4; k++){ //down side
                            if(bombers[j].position.x == bombers[i].position.x && bombers[j].position.y == bombers[i].position.y - k && j != i){
                                od bmr;
                                bmr.position = bombers[j].position;
                                bmr.type = BOMBER;
                                objects_around.push_back(bmr);
                                obj_count++;
                                break;
                            }
                        }
                    }

                    for(int j = 0; j < bomb_pids.size(); j++){ //check for bombs
                        if(bombs[j].position.x == bombers[i].position.x && bombs[j].position.y == bombers[i].position.y){
                            od bmb;
                            bmb.position = bombs[j].position;
                            bmb.type = BOMB;
                            objects_around.push_back(bmb);
                            obj_count++;
                        }
                        for(int k = 1; k < 4; k++){ //right side
                            if(bombs[j].position.x == bombers[i].position.x + k && bombs[j].position.y == bombers[i].position.y){
                                od bmb;
                                bmb.position = bombs[j].position;
                                bmb.type = BOMB;
                                objects_around.push_back(bmb);
                                obj_count++;
                                break;
                            }
                        }
                        for(int k = 1; k < 4; k++){ //left side
                            if(bombs[j].position.x == bombers[i].position.x - k && bombs[j].position.y == bombers[i].position.y){
                                od bmb;
                                bmb.position = bombs[j].position;
                                bmb.type = BOMB;
                                objects_around.push_back(bmb);
                                obj_count++;
                                break;
                            }
                        }
                        for(int k = 1; k < 4; k++){ //up side
                            if(bombs[j].position.x == bombers[i].position.x && bombs[j].position.y == bombers[i].position.y + k){
                                od bmb;
                                bmb.position = bombs[j].position;
                                bmb.type = BOMB;
                                objects_around.push_back(bmb);
                                obj_count++;
                                break;
                            }
                        }
                        for(int k = 1; k < 4; k++){ //down side
                            if(bombs[j].position.x == bombers[i].position.x && bombs[j].position.y == bombers[i].position.y - k){
                                od bmb;
                                bmb.position = bombs[j].position;
                                bmb.type = BOMB;
                                objects_around.push_back(bmb);
                                obj_count++;
                                break;
                            }
                        }
                    }

                    out.data.object_count = obj_count;

                    ret =  write(bomber_fd[i][0], &out, sizeof(om));
                    if(ret == -1){
                        perror("write");
                        exit(1);
                    }
                    omp omtp;
                    omtp.pid = bomber_pids[i];
                    omtp.m = &out;
                    print_output(NULL, &omtp, NULL, objects_around.data());

                    for(int j = 0; j < obj_count; j++){
                        //cerr << "Bomber " << i << " sees obstacle at location: " << objects_around[j].position.x << " " << objects_around[j].position.y << endl;
                        ret =  write(bomber_fd[i][0], &objects_around[j], sizeof(od));
                        if(ret == -1){
                            perror("write");
                            exit(1);
                        }
                    }
                    
                }

                else if(in.type == BOMBER_MOVE){
                    out.type = BOMBER_LOCATION;
                    if((in.data.target_position.x == bombers[i].position.x +1 && in.data.target_position.y == bombers[i].position.y) 
                        || (in.data.target_position.x == bombers[i].position.x -1 && in.data.target_position.y == bombers[i].position.y)
                        || (in.data.target_position.x == bombers[i].position.x && in.data.target_position.y == bombers[i].position.y +1)
                        || (in.data.target_position.x == bombers[i].position.x && in.data.target_position.y == bombers[i].position.y -1)){
                        
                        bool occupied = false;
                        for(int j = 0; j < obstacle_count; j++){
                            if(obstacles[j].position.x == in.data.target_position.x && obstacles[j].position.y == in.data.target_position.y){
                                occupied = true;
                                break;
                            }
                        }
                        for(int j = 0; j < bomber_count; j++){
                            if(bombers[j].position.x == in.data.target_position.x && bombers[j].position.y == in.data.target_position.y && j != i){
                                occupied = true;
                                break;
                            }
                        }

                        if(!occupied && in.data.target_position.x >= 0 && in.data.target_position.x < map_width && in.data.target_position.y >= 0 && in.data.target_position.y < map_height){
                            bombers[i].position = in.data.target_position;
                            out.data.new_position = bombers[i].position;
                        }
                        else{
                            out.data.new_position = bombers[i].position;
                        }                        
                    }
                    else{
                        out.data.new_position = bombers[i].position;
                    }

                    ret =  write(bomber_fd[i][0], &out, sizeof(om));
                    if(ret == -1){
                        perror("write");
                        exit(1);
                    }
                    omp omtp;
                    omtp.pid = bomber_pids[i];
                    omtp.m = &out;
                    print_output(NULL, &omtp, NULL, NULL);
                }
                else if(in.type == BOMBER_PLANT){
                    out.type = BOMBER_PLANT_RESULT;
                    bool bomb_exists = false;
                    for(int j=0; j<bombs.size(); j++){
                        if(bombs[j].position.x == bombers[i].position.x && bombs[j].position.y == bombers[i].position.y){
                            bomb_exists = true;
                            break;
                        }
                    }
                    if(bomb_exists){
                        out.data.planted = false;
                    }
                    else{
                        //cerate pipes for bomb
                        int bomb_fd[2];
                        if(PIPE(bomb_fd) == -1){
                            perror("pipe");
                            exit(1);
                        }
                        bomb_fds.push_back(bomb_fd);
                        //create poll for bomb
                        pollfd bomb_poll;
                        bomb_poll.fd = bomb_fd[0];
                        bomb_poll.events = POLLIN;
                        bomb_poll_fds.push_back(bomb_poll);
                        
                        bomb_pids.push_back(fork());

                        if(bomb_pids.back() == 0){
                            close(bomb_fds.back()[0]);
                            dup2(bomb_fds.back()[1], 1);
                            dup2(bomb_fds.back()[1], 0);
                            const char* bomb_args[] = {"./bomb", to_string(in.data.bomb_info.interval).c_str(), NULL };
                            execv(bomb_args[0], const_cast<char* const*>(bomb_args));
                            perror("execv");
                            exit(1);
                        }
                        else{
                            close(bomb_fds.back()[1]);
                            out.data.planted = true;
                            bomb_datas this_bomb;
                            this_bomb.position = bombers[i].position;
                            this_bomb.radius = in.data.bomb_info.radius;
                            bombs.push_back(this_bomb);
                        }
                    }
                    ret =  write(bomber_fd[i][0], &out, sizeof(om));
                    if(ret == -1){
                        perror("write");
                        exit(1);
                    }
                    omp omtp;
                    omtp.pid = bomber_pids[i];
                    omtp.m = &out;
                    print_output(NULL, &omtp, NULL, NULL);
                }
            }
        }
        //cerr << "bomb count: " << bombs.size() << endl;
        ret = poll(bomb_poll_fds.data(), bombs.size(),1000);
        if (ret == -1)
        {
            perror("poll");
            exit(1);
        }
        
        for(int i=0;i<bomb_fds.size();i++){ //read bomb messages
            if(bomb_poll_fds[i].revents & POLLIN){ 
                im in;
                ret = read(bomb_fds[i][0], &in, sizeof(im));
                if(ret == -1){
                    perror("read");
                    exit(1);
                }   

                imp imtp;
                imtp.pid = bomb_pids[i];
                imtp.m = &in;
                print_output(&imtp, NULL, NULL, NULL);
                
                if(in.type == BOMB_EXPLODE){
                    for(int k=0;k<bomber_count;k++){ //exact position
                        if(bombers[k].position.x == bombs[i].position.x && bombers[k].position.y == bombs[i].position.y){ 
                            bomber_dead[k] = true;
                            alive_bombers--;
                            if(alive_bombers == 0){
                                bomber_win[k] = true;
                                game_over = true;
                            }
                        }
                    }
                    bool right = false;
                    bool left = false;
                    bool up = false;
                    bool down = false;
                    for(int j=1;j<=bombs[i].radius;j++){
                        for(int k=0;k<obstacles.size();k++){ //right side obstacles
                            if(obstacles[k].position.x == bombs[i].position.x + j && obstacles[k].position.y == bombs[i].position.y){
                                obstacles[k].remaining_durability--;
                                if(obstacles[k].remaining_durability == 0){
                                    obstacles.erase(obstacles.begin() + k);
                                }
                                right = true;
                            }
                        }
                        for(int k=0;k<obstacles.size();k++){ //left side obstacles
                            if(obstacles[k].position.x == bombs[i].position.x - j && obstacles[k].position.y == bombs[i].position.y){
                                obstacles[k].remaining_durability--;
                                if(obstacles[k].remaining_durability == 0){
                                    obstacles.erase(obstacles.begin() + k);
                                }
                                left = true;
                            }
                        }
                        for(int k=0;k<obstacles.size();k++){ //up side obstacles
                            if(obstacles[k].position.x == bombs[i].position.x && obstacles[k].position.y == bombs[i].position.y + j){
                                obstacles[k].remaining_durability--;
                                if(obstacles[k].remaining_durability == 0){
                                    obstacles.erase(obstacles.begin() + k);
                                }
                                up = true;
                            }
                        }
                        for(int k=0;k<obstacles.size();k++){ //down side obstacles
                            if(obstacles[k].position.x == bombs[i].position.x && obstacles[k].position.y == bombs[i].position.y - j){
                                obstacles[k].remaining_durability--;
                                if(obstacles[k].remaining_durability == 0){
                                    obstacles.erase(obstacles.begin() + k);
                                }   
                                down = true;
                            }
                        }
                        for(int k=0;k<bomber_count  && !right;k++){ //right side
                            if(bombers[k].position.x == bombs[i].position.x + j && bombers[k].position.y == bombs[i].position.y){
                                bomber_dead[k] = true;
                                alive_bombers--;
                                if(alive_bombers == 0 && !game_over){
                                    bomber_win[k] = true;
                                    game_over = true;
                                }
                            }
                        }
                        
                        for(int k=0;k<bomber_count && !left;k++){ //left side
                            if(bombers[k].position.x == bombs[i].position.x - j && bombers[k].position.y == bombs[i].position.y){
                                bomber_dead[k] = true;
                                alive_bombers--;
                                if(alive_bombers == 0 && !game_over){
                                    bomber_win[k] = true;
                                    game_over = true;
                                }
                            }
                        }
                        for(int k=0;k<bomber_count && !up;k++){ //up side
                            if(bombers[k].position.x == bombs[i].position.x && bombers[k].position.y == bombs[i].position.y + j){
                                bomber_dead[k] = true;
                                alive_bombers--;
                                if(alive_bombers == 0 && !game_over){
                                    bomber_win[k] = true;
                                    game_over = true;
                                }
                            }
                        }
                        for(int k=0;k<bomber_count && !down;k++){ //down side
                            if(bombers[k].position.x == bombs[i].position.x && bombers[k].position.y == bombs[i].position.y - j){
                                bomber_dead[k] = true;
                                alive_bombers--;
                                if(alive_bombers == 0 && !game_over){
                                    bomber_win[k] = true;
                                    game_over = true;
                                }
                            }
                        }
                    }
                    waitpid(bomb_pids[i], NULL, 0);
                    bomb_pids.erase(bomb_pids.begin() + i);
                    bombs.erase(bombs.begin() + i);
                    bomb_fds.erase(bomb_fds.begin() + i);
                    i=0;
                }
            }
        }
        sleep(1);
    }   
    return 0;
}