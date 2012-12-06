//=============================================================================================================
/**
* @file     fwd.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    ToDo Documentation...
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd.h"

#include "../fs/colortable.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FWDLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Fwd::Fwd()
{
}


//*************************************************************************************************************

bool Fwd::clusterFwd(const MNEForwardSolution *p_fwdIn, MNEForwardSolution *p_fwdOut,
                     Annotation* p_pLHAnnotation, Annotation* p_pRHAnnotation, qint32 p_iClusterSize)
{
    if(p_fwdOut)
        delete p_fwdOut;
    p_fwdOut = new MNEForwardSolution(p_fwdIn);

    QList<Annotation*> t_listAnnotation;
    t_listAnnotation.append(p_pLHAnnotation);
    t_listAnnotation.append(p_pRHAnnotation);


//        p_ClusteredForwardSolution = sl_CForwardSolution(obj);

//        t_LF_new = [];
//        t_src_new(1,1).rr = [];
//        t_src_new(1,1).nn = [];
//        t_src_new(1,1).vertno = [];

//        t_src_new(1,2).rr = [];
//        t_src_new(1,2).nn = [];
//        t_src_new(1,2).vertno = [];


    for(qint32 h = 0; h < p_fwdIn->src->hemispheres.size(); ++h )//obj.sizeForwardSolution)
    {
        if(h == 0)
            printf("Cluster Left Hemisphere\n");
        else
            printf("Cluster Right Hemisphere\n");

        Colortable* t_pCurrentColorTable = t_listAnnotation[h]->getColortable();
        VectorXi label = t_pCurrentColorTable->getAvailableROIs();
        for (qint32 i = 0; i < label.rows(); ++i)
        {
            if (label[i] != 0)
            {
                QString curr_name = t_pCurrentColorTable->struct_names[i];//obj.label2AtlasName(label(i));
                printf("\tCluster %d / %d %s\n", i+1, label.rows(), curr_name.toUtf8().constData());

//                    if h == 1
//                        curr_name = [curr_name(1,1).names{1,1} ' left hemisphere'];
//                        disp(['Cluster ' num2str(i) '/' num2str(length(label)) ', ' curr_name]);
//                        p_ClusteredForwardSolution.selectROIs('lh',label(i),'rh',[]);
//                    else
//                        curr_name = [curr_name(1,2).names{1,1} ' right hemisphere'];
//                        disp(['Cluster ' num2str(i) '/' num2str(length(label)) ', ' curr_name]);
//                        p_ClusteredForwardSolution.selectROIs('lh',[],'rh',label(i));
//                    end

//                    t_LF = p_ClusteredForwardSolution.data;

//                    [t_iSensors, t_iSources_p] = size(t_LF);

//                    if t_iSources_p > 0
//                        t_iSources = t_iSources_p/3;
//                        t_iClusters = ceil(t_iSources/p_iClusterSize);

//                        fprintf('%d Cluster(s)\n', t_iClusters);

//                        t_LF_partial = zeros(t_iSensors,t_iClusters*3);

//                        for j = 1:t_iSensors %parfor
//                            t_sensLF = reshape(t_LF(j,:),3,[]);
//                            t_sensLF = t_sensLF';

//                            % Kmeans Reduction
//                            opts = statset('Display','off');%'final');
//%                                     %Euclidean
//%                                     [idx, ctrs] = kmeans(t_sensLF,t_iClusters,...
//%                                                         'Distance','sqEuclidean',...
//%                                                         'Replicates',5,...
//%                                                         'Options',opts);
//%                                     %Correlation
//%                                     [idx, ctrs] = kmeans(t_sensLF,t_iClusters,...
//%                                                         'Distance','correlation',...
//%                                                         'Replicates',5,...
//%                                                         'Options',opts);
//                            %L1
//                            [idx, ctrs] = kmeans(t_sensLF,t_iClusters,...
//                                                'Distance','cityblock',...
//                                                'Replicates',5,...
//                                                'Options',opts);
//%                                     [idx, ctrs] = kmeans(t_sensLF,t_iClusters,...
//%                                                         'Distance','cosine',...
//%                                                         'Replicates',5,...
//%                                                         'Options',opts);


//                            ctrs_straight = ctrs';
//                            t_LF_partial(j,:) = ctrs_straight(:)';




//                            %Plot %% ToDo problem with ParFor
//                            if ~isempty(find(ismember(p_vecPlotSensors, j),1)) %plot the selected sensor
//                                %Before Clustering
//                                if h == 1
//                                    color = obj.label2Color('lh',label(i));
//                                else
//                                    color = obj.label2Color('rh',label(i));
//                                end
//                                figure('Name', 'Before Clustering');
//                                plot3(t_sensLF(:,1), t_sensLF(:,2), t_sensLF(:,3), 'o', 'MarkerSize', 6, 'MarkerFaceColor', color./255);
//                                title(['Sensor ' num2str(j) ' ' strrep(curr_name, '_', ' ')]);
//                                axis equal

//                                %After Clustring
//                                figure('Name', 'After Clustering');
//                                color_map = jet(t_iClusters);
//                                marker = ['+'; 'o';'s';'*';'.';'x';'d';'^';'v'];
//                                for k = 1:t_iClusters
//                                    plot3(t_sensLF(idx==k,1),t_sensLF(idx==k,2),t_sensLF(idx==k,3),marker(k),'MarkerEdgeColor',color./255,'MarkerFaceColor',color./255,'MarkerSize',6);%'MarkerEdgeColor',color_map(k,:),'LineWidth',2,
//                                    axis equal
//                                    hold on
//                                end
//                                plot3(ctrs(:,1),ctrs(:,2),ctrs(:,3),'x',...
//                                    'MarkerEdgeColor',color./255,'MarkerSize',12,'LineWidth',2)
//                                title(['Clustered Sensor ' num2str(j) ' ' strrep(curr_name, '_', ' ')]);
//                                sl_CUtility.ArrFig('Region', 'fullscreen', 'figmat', [], 'distance', 20, 'monitor', 1);
//                                pause;
//                                close all;
//                            end
//                        end

//                        %Assign clustered data to new LeadField
//                        t_LF_new = [t_LF_new t_LF_partial];

//                        for k = 1:t_iClusters
//                            [~, n] = size(t_LF);
//                            nSources = n/3;
//                            idx = (k-1)*3+1;
//                            t_LF_partial_resized = repmat(t_LF_partial(:,idx:idx+2),1,nSources);
//                            t_LF_diff = sum(abs(t_LF-t_LF_partial_resized),1);

//                            t_LF_diff_dip = [];
//                            for l = 1:nSources
//                                idx = (l-1)*3+1;
//                                t_LF_diff_dip = [t_LF_diff_dip sum(t_LF_diff(idx:idx+2))];
//                            end

//                            %Take the closest coordinates
//                            sel_idx = ismember(t_LF_diff_dip, min(t_LF_diff_dip));
//                            rr = p_ClusteredForwardSolution.src(1,h).rr(sel_idx,:);
//                            nn = [0 0 0];
//                            t_src_new(1,h).rr = [t_src_new(1,h).rr; rr];
//                            t_src_new(1,h).nn = [t_src_new(1,h).nn; nn];

//                            rr_idx = ismember(p_ClusteredForwardSolution.defaultSolutionSourceSpace.src(1,h).rr, rr);
//                            vertno_idx = rr_idx(:,1) & rr_idx(:,2) & rr_idx(:,3);
//                            t_src_new(1,h).vertno = [t_src_new(1,h).vertno p_ClusteredForwardSolution.defaultSolutionSourceSpace.src(1,h).vertno(vertno_idx)];
//                        end
//                    end
            }
        }
        printf("[done]\n");
    }

//        %set new stuff;
//        p_ClusteredForwardSolution.m_ForwardSolution.sol.data = t_LF_new;
//        [p_ClusteredForwardSolution.m_ForwardSolution.sol.nrow,...
//            p_ClusteredForwardSolution.m_ForwardSolution.sol.ncol] = size(t_LF_new);

//        p_ClusteredForwardSolution.m_ForwardSolution.nsource = p_ClusteredForwardSolution.m_ForwardSolution.sol.ncol/3;
//        p_ClusteredForwardSolution.m_ForwardSolution.nchan = p_ClusteredForwardSolution.m_ForwardSolution.sol.nrow;

//        source_rr = [];
//        for h = 1:obj.sizeForwardSolution
//            p_ClusteredForwardSolution.m_ForwardSolution.src(1,h).vertno = t_src_new(1,h).vertno;
//            p_ClusteredForwardSolution.m_ForwardSolution.src(1,h).nuse = length(t_src_new(1,h).vertno);
//            source_rr = [source_rr; t_src_new(1,h).rr];
//            p_ClusteredForwardSolution.m_ForwardSolution.src(1,h).nuse_tri = 0;
//            p_ClusteredForwardSolution.m_ForwardSolution.src(1,h).use_tris = [];
//        end
//        p_ClusteredForwardSolution.m_ForwardSolution.source_rr = source_rr;
//        p_ClusteredForwardSolution.m_ForwardSolution.source_nn = ...
//            p_ClusteredForwardSolution.m_ForwardSolution.source_nn(1:p_ClusteredForwardSolution.m_ForwardSolution.sol.ncol,:);


//        p_ClusteredForwardSolution.resetROISelection();
//        p_ClusteredForwardSolution.resetSourceSelection();





    return true;
}
