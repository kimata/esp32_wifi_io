import { Component, OnInit } from '@angular/core';
import { HttpClient, HttpParams  } from '@angular/common/http';
import { ToastrService } from 'ngx-toastr';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.scss']
})
export class AppComponent implements OnInit {
    constructor(
        private http: HttpClient,
        private toastrService: ToastrService,
    ) { }
    
    public version = '0.0.1';
    public gpio_list = [ 32, 33, 25, 26 ];
    public app_info: any = {
        name: '?',
        version: '?',
        esp_idf: '?',
        compile_date: '?',
        compile_time: '?',
        elapse: '?',
    };

    ngOnInit() {
        this.updateAppInfo();
    }

    updateAppInfo() {
        this.http.get('/status/').subscribe(
            json => {
                this.app_info = json;
            },
            error => {
                // ignore
            }
        );
    }

    buttonClick(gpio) {
        this.http.get('/api/gpio/push/' + gpio).subscribe(
            json => {
                if (json["status"] == "OK") {
                    this.toastrService.success('正常に制御できました．', '成功');
                } else {
                    this.toastrService.error('制御に失敗しました．', '失敗'); 
                }
            },
            error => {
                this.toastrService.error('制御に失敗しました．', '失敗'); 
            }
        );
    }
}
