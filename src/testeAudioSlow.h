if(sampleCount > 0){
                    //decodedFec = opusMng.decodeC((const unsigned char*)bufferOpus.data(), bufferOpus.size(), sampleCount, 1);
                    decoded = opusMng.decode((const unsigned char*)bufferOpus.data(), bufferOpus.size(), sampleCount);
                    sf::Int16 * decRI = reinterpret_cast<sf::Int16*>(decoded.getData());
                    decodedSF.insert(decodedSF.end(), decRI, decRI + (decoded.size() / sizeof(sf::Int16)));

                    float gainVelocity = 1.0;
                    float gainPitch = 1.0;
                    float calcPerc = 0.1;
                    gainPitch -= calcPerc;
                    gainVelocity += calcPerc;

                    //setPitch(gainVelocity);

                    float** insamples = new float*[1];
                    insamples[0] = new float[decodedSF.size()];

                    std::vector<std::vector<float>> inputVec;
                    inputVec.resize(1);
                    inputVec[0].resize(decodedSF.size());

                    std::vector<float> insamplesBuff;
                    insamplesBuff.resize(decodedSF.size());

                    std::vector<sf::Int16> testBuff;
                    testBuff.insert(testBuff.begin(), decodedSF.begin(), decodedSF.end());
                    
                    for(int i = 0; i < (int)testBuff.size(); i++){
                        testBuff[i] =  testBuff[i]+(pow(testBuff[i],2)/(testBuff[i]*32));
                    }

                    float copysample;
                    for(int i = 0; i < (int)decodedSF.size(); i++){
                        copysample = ((float)decodedSF[i]) / (float)32768.0F;
                        if( copysample > 1.0F ) copysample = 1.0F;
                        if( copysample < -1.0F ) copysample = -1.0F;

                        insamples[0][i] = copysample;
                        inputVec[0][i] = copysample;
                        insamplesBuff[i] = copysample;
                    }

                    float** outputBuff = new float*[1];
                    outputBuff[0] = new float[decodedSF.size()];
                    int outputBuff_size = decodedSF.size();
                    std::vector<sf::Int16> outputVect;

                    std::vector<std::vector<float>> outputVectB;
                    outputVectB.resize(1);
                    outputVectB[0].resize(decodedSF.size());

                    stretch.configure(sampleChannels, decodedSF.size(), decodedSF.size()/4);
                    stretch.process(inputVec, decodedSF.size(), outputVectB, decodedSF.size() - 100);

                    stretch.reset();

                    /*
                    soundTouch.putSamples(reinterpret_cast<const float*>(insamples), decodedSF.size());
                    soundTouch.setTempo(1.3);
                    soundTouch.flush();

                    while (soundTouch.numSamples() > 0 || soundTouch.numUnprocessedSamples() > 0) {
                        int bufferSize = soundTouch.numSamples();
                        float* outputBuffer = new float[bufferSize];
                        int numSamplesProcessed = soundTouch.receiveSamples(outputBuffer, bufferSize);

                        outputBuff.insert(outputBuff.end(), outputBuffer, outputBuffer + bufferSize);

                        delete[] outputBuffer;
                    }

                    soundTouch.clear();
                    */

                    outputVect.resize(outputBuff_size);

                    for(int i = 0; i < outputBuff_size; i++){
                        outputBuff[0][i] = (outputBuff[0][i] * 32768.0F);
                        if( outputBuff[0][i] > 32767.0F ) outputVect[i] = 32767.0F;
                        if( outputBuff[0][i] < -32768.0F ) outputVect[i] = -32768.0F;
                        outputVect[i] = (sf::Int16)outputBuff[0][i];
                    }

                    delete[] insamples;
                    delete[] outputBuff;

                    //opusQueue.clearFrames(readPosActual, lastPush + 1);
                    //opusQueue.setReadPos(lastPush);
                    std::cout << "Audio advance " << sampleTimeInQueue << ":" << maxSampleTimeInQueue << "/" << initDelayCountMS << std::endl;

                    data.samples = testBuff.data();
                    data.sampleCount = testBuff.size();
                    return true;
                }