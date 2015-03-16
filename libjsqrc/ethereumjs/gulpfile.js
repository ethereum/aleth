#!/usr/bin/env node

'use strict';

var version = require('./version.json');
var path = require('path');

var del = require('del');
var gulp = require('gulp');
var browserify = require('browserify');
var jshint = require('gulp-jshint');
var uglify = require('gulp-uglify');
var rename = require('gulp-rename');
var source = require('vinyl-source-stream');
var exorcist = require('exorcist');
var bower = require('bower');
var streamify = require('gulp-streamify');
var replace = require('gulp-replace');

var DEST = './dist/';
var src = 'index';
var dst = 'ethereum';

var browserifyOptions = {
    debug: true,
    insert_global_vars: false, // jshint ignore:line
    detectGlobals: false,
    bundleExternal: false
};

gulp.task('versionReplace', function(){
  gulp.src(['./package.json'])
    .pipe(replace(/\"version\"\: \"(.{5})\"/, '"version": "'+ version.version + '"'))
    .pipe(gulp.dest('./'));
  gulp.src(['./package.js'])
    .pipe(replace(/version\: \'(.{5})\'/, "version: '"+ version.version + "'"))
    .pipe(gulp.dest('./'));
});

gulp.task('bower', function(cb){
    bower.commands.install().on('end', function (installed){
        console.log(installed);
        cb();
    });
});

gulp.task('clean', ['lint'], function(cb) {
    del([ DEST ], cb);
});

gulp.task('lint', function(){
    return gulp.src(['./*.js', './lib/*.js'])
        .pipe(jshint())
        .pipe(jshint.reporter('default'));
});

gulp.task('build', ['clean'], function () {
    return browserify(browserifyOptions)
        .require('./' + src + '.js', {expose: 'web3'})
        .add('./' + src + '.js')
        .bundle()
        .pipe(exorcist(path.join( DEST, dst + '.js.map')))
        .pipe(source(dst + '.js'))
        .pipe(gulp.dest( DEST ))
        .pipe(streamify(uglify()))
        .pipe(rename(dst + '.min.js'))
        .pipe(gulp.dest( DEST ));
});

gulp.task('watch', function() {
    gulp.watch(['./lib/*.js'], ['lint', 'build']);
});

gulp.task('dev', ['versionReplace','bower', 'lint', 'build']);
gulp.task('default', ['dev']);


gulp.task('version', ['versionReplace']);

